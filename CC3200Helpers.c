#include "simplelink.h"
#include "defs.h"
#include "CC3200Helpers.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "rom.h"
#include "rom_map.h"
#include "hw_memmap.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
#include "spi.h"

static volatile unsigned char s_flWLANConnected = 0;
static volatile unsigned char s_flAcquiredIP = 0;
static volatile SlWlanMode_t s_eRequestedWlanMode = ROLE_STA;

void CC3200Helpers_Delay(unsigned long dwMicroSeconds)
{
  UtilsDelay(80*dwMicroSeconds/3);
}


static int cc3200helpers_start(int *piMode)
{
  *piMode = sl_Start(NULL,NULL,NULL);

  switch ( *piMode )
  {
  case ROLE_STA:
  case ROLE_AP:
  case ROLE_P2P:
    return cc3200Helpers_OK;
  case ROLE_STA_ERR: return cc3200Helpers_StartFailInSTA;
  case ROLE_AP_ERR: return cc3200Helpers_StartFailInAP;
  case ROLE_P2P_ERR: return cc3200Helpers_StartFailInP2P;
  default: break;
  }

  return cc3200Helpers_Internal;
}

static unsigned long cc3200helpers_startIn(SlWlanMode_t eWlanMode)
{
  int iMode = 0;
  unsigned long dwResult = 0;

  s_flWLANConnected = 0;
  s_flAcquiredIP = 0;
  SlWlanMode_t s_eRequestedWlanMode = eWlanMode;

  dwResult = cc3200helpers_start(&iMode);

  if ( dwResult )
  {
    return dwResult;
  }

  if ( eWlanMode == iMode )
  {
    return cc3200Helpers_OK;
  }

  if (ROLE_AP == iMode)
  {
      // If the device is in AP mode, we need to wait for this event
      // before doing anything
      TRACE("Waiting for AP to initialize\n\r");
      while(!s_flAcquiredIP)
      {
#ifndef SL_PLATFORM_MULTI_THREADED
        _SlNonOsMainLoopTask();
#endif
      }
      s_flAcquiredIP = 0;
      TRACE("AP ready\n\r");
  }

  if ( sl_WlanSetMode(eWlanMode) )
  {
    return cc3200Helpers_SwitchFailed | cc3200Helpers_SetModeFailed;
  }

  if ( sl_Stop(0) )
  {
    return cc3200Helpers_SwitchFailed | cc3200Helpers_StopFailed;
  }

 dwResult = cc3200helpers_start(&iMode);

 if ( dwResult )
 {
   return cc3200Helpers_SwitchFailed | dwResult;
 }

 if ( iMode != eWlanMode )
 {
   return cc3200Helpers_SwitchFailed;
 }
  return cc3200Helpers_OK;
}

unsigned long CC3200Helpers_StartConfigAP()
{
  unsigned long dwResult = cc3200helpers_startIn(ROLE_AP);
  if ( dwResult )
  {
    return dwResult;
  }
  return dwResult;
}

unsigned char cc3200helpers_isProfileConfigured(unsigned char byProfile)
{
  SlSecParams_t SecParams;
  SlGetSecParamsExt_t EntParams;
  _u32 dwPriority;
  signed char achName[32 + 1] = { 0 };
  short sNameLength = sizeof(achName) - 1;
  unsigned char abyMAC[6];
  _i16 sResult = sl_WlanProfileGet(byProfile, achName, &sNameLength, abyMAC, &SecParams, &EntParams, &dwPriority);
  if ( sResult < 0 )
  {
    return 0;
  }
  return !0;
}

unsigned long CC3200Helpers_ProfileConnect(char *pszDeviceName, unsigned short wMaxNameLength)
{
  int iResult;
  unsigned char byLength = (unsigned char)wMaxNameLength;
  long lRetVal = 0;
  unsigned long dwResult;
  dwResult = cc3200helpers_startIn(ROLE_STA);
  if ( dwResult )
  {
    return dwResult;
  }
  if ( !cc3200helpers_isProfileConfigured(0) )
  {
    return cc3200Helpers_ProfileNotConfigured;
  }
  if ( pszDeviceName && wMaxNameLength )
  {
    lRetVal = sl_NetAppGet (SL_NET_APP_DEVICE_CONFIG_ID, NETAPP_SET_GET_DEV_CONF_OPT_DEVICE_URN, &byLength, (_u8 *)pszDeviceName);
  }
  return dwResult;
}

unsigned char CC3200Helpers_IsConnected()
{
#ifndef SL_PLATFORM_MULTI_THREADED
        _SlNonOsMainLoopTask();
#endif
  if ( ROLE_AP == s_eRequestedWlanMode )
  {
    return 0 != s_flAcquiredIP;
  }
  if ( ROLE_STA == s_eRequestedWlanMode )
  {
    return s_flWLANConnected && s_flAcquiredIP;
  }
  return 0;
}


void CC3200Helpers_AllowAccessToROMConfig(unsigned char flAllow)
{

}


#define SLOW_CLK_FREQ 32768
void CC3200Helpers_HibernateNowFor(unsigned long dwSeconds, char flStopSL)
{
  unsigned long long qwTicks = dwSeconds;
  qwTicks *= SLOW_CLK_FREQ;
  MAP_PRCMHibernateIntervalSet(qwTicks);
  MAP_PRCMHibernateWakeupSourceEnable(PRCM_HIB_SLOW_CLK_CTR);
  MAP_UtilsDelay(80000);
  if ( flStopSL )
  {
    sl_WlanDisconnect();
    sl_Stop(0);
  }
  MAP_SPICSDisable(SSPI_BASE);
  MAP_SPICSEnable(SSPI_BASE);
  MAP_SPIDataPut(SSPI_BASE, 0xB9); // deep power down
  MAP_SPICSDisable(SSPI_BASE);
  MAP_PRCMHibernateEnter();
}

void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
          s_flWLANConnected = 1;
          TRACE("[WLAN EVENT] STA Connected to the AP: %s\n\r", pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_name);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            s_flWLANConnected = 0;
            s_flAcquiredIP = 0;
            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
              TRACE("[WLAN EVENT]Device disconnected from the AP on application's request \n\r");
            }
            else
            {
              TRACE("[WLAN ERROR]Device disconnected from the AP AP: %s on an ERROR..!! \n\r");
            }
        }
        break;
        case SL_WLAN_STA_CONNECTED_EVENT: TRACE("Client connected\n\r"); break;
        case SL_WLAN_STA_DISCONNECTED_EVENT: TRACE("Client disconnected\n\r"); break;
        default: TRACE("[WLAN EVENT] Unexpected event [0x%x]\n\r", pWlanEvent->Event); break;
    }
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;
            s_flAcquiredIP = 1;
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;
            TRACE("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
            "Gateway=%d.%d.%d.%d\n\r",
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0));
        }
        break;

        default: TRACE("[NETAPP EVENT] Unexpected event [0x%x] \n\r", pNetAppEvent->Event); break;
    }
}


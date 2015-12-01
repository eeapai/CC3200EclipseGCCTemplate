#ifndef __CC3200_HELPERS_H__
#define __CC3200_HELPERS_H__

typedef enum _ECC3200Helpers_Result
{
  cc3200Helpers_OK                    = 0x00000000,
  cc3200Helpers_Internal              = 0x00000001, // Unknown

  cc3200Helpers_StartFailMask         = 0x00000006,
  cc3200Helpers_StartFailInSTA        = 0x00000002,
  cc3200Helpers_StartFailInAP         = 0x00000004,
  cc3200Helpers_StartFailInP2P        = 0x00000006,

  cc3200Helpers_SetModeFailed         = 0x00000008,
  cc3200Helpers_StopFailed            = 0x00000010,
  cc3200Helpers_ProfileNotConfigured  = 0x00000020,

  cc3200Helpers_SwitchFailed          = 0x00000040,


  cc3200Helpers_NeedsReset            = 0x80000000,
}ECC3200Helpers_Result;

unsigned long CC3200Helpers_StartConfigAP();
unsigned long CC3200Helpers_ProfileConnect(char *pszDeviceName, unsigned short wMaxNameLength);
unsigned char CC3200Helpers_IsConnected();
void CC3200Helpers_AllowAccessToROMConfig(unsigned char flAllow);
void CC3200Helpers_HibernateNowFor(unsigned long dwSeconds, char flStopSL);
void CC3200Helpers_Delay(unsigned long dwMicroSeconds);

#endif  //  __CC3200_HELPERS_H__

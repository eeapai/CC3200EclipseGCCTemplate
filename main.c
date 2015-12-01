#include "CC3200Helpers.h"
#include "stddef.h"
#include "simplelink.h"
#include "defs.h"

int main()
{
  unsigned long dwWD = 0;
  unsigned long dwTimeout = 30000;
  unsigned long dwResult = CC3200Helpers_ProfileConnect(NULL, 0);
  if ( cc3200Helpers_ProfileNotConfigured == dwResult )
  {
    dwResult = CC3200Helpers_StartConfigAP();
  }

  if ( cc3200Helpers_OK == dwResult )
  {
    while ( !CC3200Helpers_IsConnected() )
    {
      if ( dwWD++ > dwTimeout )
      {
        ASSERT(0);
        CC3200Helpers_HibernateNowFor(60 * 3, 1);
      }
      CC3200Helpers_Delay(1000);
    }
  }
  return 0;
}

void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent, SlHttpServerResponse_t *pHttpResponse)
{

}

void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{

}

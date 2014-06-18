#include "CModbus.h"
#include "Errors.h"
#include "functions.h"
#include <vcl.h>

DCB dcb;
HANDLE hCom;
COMMTIMEOUTS CommTimeOut;
//--------------------------------------------------------------------
CModbus::CModbus(unsigned short wAddress)
{
 this->wAddress=wAddress;
}

//--------------------------------------------------------------------
CModbus::~CModbus()
{
}

//--------------------------------------------------------------------
void CModbus::Disconnect()
{
 CloseHandle(hCom);
}
//--------------------------------------------------------------------
WORD CModbus::loopback()
{
 WORD temp;
 BYTE ByteArray[8];
 BYTE RByteArray[10];
 WORD Hi,Lo;
 unsigned long lpWbytes,lpRbytes;
 WORD Error=0;
  ByteArray[0]=this->wAddress;
  ByteArray[1]=8;
  ByteArray[2]=0;
  ByteArray[3]=0;
  ByteArray[4]=0;
  ByteArray[5]=0;
  temp=CalcCrc(ByteArray,6);
  Hi=temp>>8;
  ByteArray[6]=(BYTE)Hi;
  Lo=temp&0x00ff;
  ByteArray[7]=(BYTE)Lo;
  if (WriteFile(hCom,ByteArray,8,&lpWbytes,NULL))
  {
   PurgeComm(hCom,PURGE_RXCLEAR | PURGE_TXCLEAR);
   if(ReadFile(hCom,RByteArray,8,&lpRbytes,0))
   {
    if(lpRbytes!=0)
    {
     for (int i=0;i<8;i++) //Если полученный пакет отличается от отправленного
      if (ByteArray[i]!=RByteArray[i]) Error^=ERR_INVALID_ANSWER;
    }
    else //Если число считанных байт=0
     Error^=ERR_NO_READ_BYTES;
   }
   else //Если ошибка в ReadFile
    Error^=ERR_READFILE;
  }
  else //Если ошибка в WriteFile
   Error^=ERR_WRITEFILE;

  return (Error);
}

//--------------------------------------------------------------------
WORD CModbus::Connect(char *lpszPort,int nBaudeRate,
                int nByteSize, int nParity, int nStopBits,
                int nReadInterval)
{
 WORD Error=0;
 if (hCom !=NULL) CloseHandle(hCom);
 hCom = CreateFile(lpszPort,
                   GENERIC_READ | GENERIC_WRITE,
                   0, NULL, OPEN_EXISTING, 0,NULL);
 if (hCom == INVALID_HANDLE_VALUE) Error^=ERR_INVALIDE_HANDLE_VALUE;

 if(Error==0)
 {
  if (!GetCommState(hCom, &dcb)) Error^=ERR_NO_GET_COMMSTATE;

  if (Error==0)
  {
   dcb.BaudRate = nBaudeRate;
   dcb.ByteSize =nByteSize;
   dcb.Parity=nParity;
   dcb.StopBits=nStopBits;
   if(!SetCommState(hCom, &dcb)) Error^=ERR_NO_SET_COMMSTATE;


   if (!GetCommTimeouts(hCom,&CommTimeOut)) Error^=ERR_NO_GET_COMMTIMEOUTS;

   if (Error==0)
   {
     CommTimeOut.ReadIntervalTimeout=nReadInterval;
     CommTimeOut.ReadTotalTimeoutMultiplier=nReadInterval;
     CommTimeOut.ReadTotalTimeoutConstant=nReadInterval;
     if(!SetCommTimeouts(hCom,&CommTimeOut)) Error^=ERR_NO_SET_COMMTIMEOUTS;
   }
  }
 }
 return Error;
}

//--------------------------------------------------------------------

WORD CModbus::CalcCrc (UCHAR* Str, WORD NumBytes)
{
 WORD crc=0xFFFF;
 WORD wCrc;
 UCHAR i,j;

   for (i=0; i<NumBytes; i++) {
       crc ^= Str[i];
       for (j=0; j<8; j++) {
           if (crc & 1) {
              crc >>= 1;
              crc ^= 0xA001;
           }
           else
              crc >>= 1;
       }
   }

   wCrc=((WORD)LOBYTE(crc))<<8;
   wCrc=wCrc|((WORD)HIBYTE(crc));
   return (wCrc);
}

//--------------------------------------------------------------------
WORD CModbus::Write(BYTE *wpData,ULONG ulWriteByte)
{
 BYTE RByteArray[100];
 BYTE WByteArray[8];
 unsigned long ulWbytes,ulRbytes;
 WORD Error=0;
 for (int i=0; i<8; i++)
  WByteArray[i]=wpData[i];
 if (WriteFile(hCom,WByteArray,8,&ulWbytes,NULL))
  {
   PurgeComm(hCom,PURGE_RXCLEAR | PURGE_TXCLEAR);
   if(ReadFile(hCom,RByteArray,8,&ulRbytes,0))
   {
    if(ulRbytes!=0)
    {
     for(int i=0;i<8;i++)
      if(WByteArray[i]!=RByteArray[i]) Error^=ERR_INVALID_ANSWER;
    }
    else //Если число считанных байт=0
     Error^=ERR_NO_READ_BYTES;
   }
   else //Если ошибка в ReadFile
    Error^=ERR_READFILE;
  }
  else //Если ошибка в WriteFile
   Error^=ERR_WRITEFILE;

  return Error;
}

//--------------------------------------------------------------------
WORD CModbus::Read(BYTE *wpData, WORD *wpResult)
{
 BYTE cRByteArray[8];
 BYTE cWByteArray[8];
 WORD Error;
 WORD temp;
 unsigned long ulRbytes,ulWbytes;
 for (int i=0; i<8; i++)
  cWByteArray[i]=wpData[i];
  if (WriteFile(hCom,cWByteArray,8,&ulWbytes,NULL))
  {
   PurgeComm(hCom,PURGE_RXCLEAR | PURGE_TXCLEAR);
   if(ReadFile(hCom,cRByteArray,8,&ulRbytes,0))
   {
    if(ulRbytes!=0)
    {
     temp=CalcCrc(cRByteArray,(WORD)ulRbytes-2);
     if (cRByteArray[ulRbytes-1]==(BYTE)temp &&
         cRByteArray[ulRbytes-2]==temp>>8)
     {
       wpResult[0]=cRByteArray[3];
       cRByteArray[2]--;
       if (cRByteArray[2]>0)
       {
        wpResult[0]<<=8;
        wpResult[0]+=cRByteArray[4];
       }
     }
     else
      Error^=ERR_CRC_FAILED;
    }
    else //Если число считанных байт=0
     Error^=ERR_NO_READ_BYTES;
   }
   else //Если ошибка в ReadFile
    Error^=ERR_READFILE;
  }
  else //Если ошибка в WriteFile
   Error^=ERR_WRITEFILE;

  return Error;
}
//--------------------------------------------------------------------
WORD CModbus::ReadDiscreteOutputOrCoil(WORD wAddressCoil,WORD *wResult)
{
 BYTE WByteArray[8];
 WORD Error=0;
 WORD *pResult=new WORD[1];
 WORD temp;
 WByteArray[0]=this->wAddress;
 WByteArray[1]=READ_COIL;
 wAddressCoil--;
 temp=wAddressCoil>>8;
 WByteArray[2]=temp;
 temp=(BYTE)wAddressCoil;
 WByteArray[3]=temp;
 WByteArray[4]=0;
 WByteArray[5]=1;
 temp=CalcCrc(WByteArray,6);
 WByteArray[6]=temp>>8;
 WByteArray[7]=(BYTE)temp;
 Error=Read(WByteArray,pResult);
 wResult[0]=pResult[0];
 return Error;
}

//--------------------------------------------------------------------
WORD CModbus::ReadDiscreteInput(WORD wAddressInput,WORD *wResult)
{
 BYTE WByteArray[8];
 WORD Error=0;
 WORD *pResult=new WORD[1];
 WORD temp;
 WByteArray[0]=this->wAddress;
 WByteArray[1]=READ_DISCRETE_INPUT;
 wAddressInput--;
 temp=wAddressInput>>8;
 WByteArray[2]=temp;
 temp=(BYTE)wAddressInput;
 WByteArray[3]=temp;
 WByteArray[4]=0;
 WByteArray[5]=1;
 temp=CalcCrc(WByteArray,6);
 WByteArray[6]=temp>>8;
 WByteArray[7]=(BYTE)temp;
 Error=Read(WByteArray,pResult);
 wResult[0]=pResult[0];
 return Error;
}

//--------------------------------------------------------------------
WORD CModbus::ReadInputRegister(WORD wAddressInputReg, WORD *wResult)
{
 BYTE WByteArray[8];
 WORD Error=0;
 WORD *pResult=new WORD[1];
 WORD temp;
 WByteArray[0]=this->wAddress;
 WByteArray[1]=READ_INPUT_REGISTER;
 wAddressInputReg--;
 temp=wAddressInputReg>>8;
 WByteArray[2]=temp;
 temp=(BYTE)wAddressInputReg;
 WByteArray[3]=temp;
 WByteArray[4]=0;
 WByteArray[5]=1;
 temp=CalcCrc(WByteArray,6);
 WByteArray[6]=temp>>8;
 WByteArray[7]=(BYTE)temp;
 Error=Read(WByteArray,pResult);
 wResult[0]=pResult[0];
 return Error;
}

//--------------------------------------------------------------------
WORD CModbus::ReadHoldingRegister(WORD wAddressHoldReg, WORD *wResult)
{
 BYTE WByteArray[8];
 WORD Error=0;
 WORD *pResult=new WORD[1];
 WORD temp;
 WByteArray[0]=this->wAddress;
 WByteArray[1]=READ_HOLDING_REGISTER;
 wAddressHoldReg--;
 temp=wAddressHoldReg>>8;
 WByteArray[2]=temp;
 temp=(BYTE)wAddressHoldReg;
 WByteArray[3]=temp;
 WByteArray[4]=0;
 WByteArray[5]=1;
 temp=CalcCrc(WByteArray,6);
 WByteArray[6]=temp>>8;
 WByteArray[7]=(BYTE)temp;
 Error=Read(WByteArray,pResult);
 wResult[0]=pResult[0];
 return Error;
}
//--------------------------------------------------------------------
WORD CModbus::WriteSingleCoil(WORD wAddressCoil, BOOL Coil)
{
 BYTE WByteArray[8];
 WORD Error=0;
 WORD temp;
 WByteArray[0]=this->wAddress;
 WByteArray[1]=WRITE_SINGLE_COIL;
 wAddressCoil--;
 temp=wAddressCoil>>8;
 WByteArray[2]=temp;
 temp=(BYTE)wAddressCoil;
 WByteArray[3]=temp;
 if (Coil)
  WByteArray[4]=0xff;
 else
  WByteArray[4]=0;
 WByteArray[5]=0;
 temp=CalcCrc(WByteArray,6);
 WByteArray[6]=temp>>8;
 WByteArray[7]=(BYTE)temp;
 Error=Write(WByteArray,8);
 return Error;
}

//--------------------------------------------------------------------
WORD CModbus::WriteSingleRegister(WORD wAddressHoldReg, WORD wRegValue)
{
BYTE WByteArray[8];
 WORD Error=0;
 WORD temp;
 WByteArray[0]=this->wAddress;
 WByteArray[1]=WRITE_SINGLE_REGISTER;
 wAddressHoldReg--;
 temp=wAddressHoldReg>>8;
 WByteArray[2]=temp;
 temp=(BYTE)wAddressHoldReg;
 WByteArray[3]=temp;
 temp=wRegValue>>8;
 WByteArray[4]=temp;
 temp=(BYTE)wRegValue;
 WByteArray[5]=temp;
 temp=CalcCrc(WByteArray,6);
 WByteArray[6]=temp>>8;
 WByteArray[7]=(BYTE)temp;
 Error=Write(WByteArray,8);
 return Error;
}

//--------------------------------------------------------------------
void CModbus::FloatToHex(float Data, WORD *Result)
{
 BYTE *pFloat;
 WORD wTemp;
 pFloat=(unsigned char *)&Data;
 wTemp=pFloat[1];
 wTemp<<=8;
 wTemp+=pFloat[0];
 Result[0]=wTemp;
 wTemp=pFloat[3];
 wTemp<<=8;
 wTemp+=pFloat[2];
 Result[1]=wTemp;
}

//--------------------------------------------------------------------
float CModbus::HexToFloat(WORD *Data)
{
 float rTemp;
 WORD  wTemp;
 BYTE *pFloat;
 pFloat=(unsigned char *)&rTemp;
 wTemp=Data[0];
 pFloat[1]=wTemp>>8;
 pFloat[0]=wTemp;
 wTemp=Data[1];
 pFloat[3]=wTemp>>8;
 pFloat[2]=wTemp;
 return rTemp;
}
//--------------------------------------------------------------------


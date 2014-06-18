#include <vcl.h>
class CModbus{
 public:
  CModbus(WORD wAddress); //Конструктор
  ~CModbus(); //Деструктор
  WORD Connect(char *lpszPort,int nBaudeRate,
                int nByteSize, int nParity, int nStopBits,
                int nReadInterval);
  WORD CalcCrc(UCHAR *Str,WORD NumBytes);
  WORD loopback();
  void CModbus::Disconnect(); 
  WORD ReadDiscreteOutputOrCoil(WORD wAddressCoil, WORD *wResult);
  WORD ReadDiscreteInput(WORD wAddressInput, WORD *wResult);
  WORD ReadInputRegister(WORD wAddressInputReg, WORD *wResult);
  WORD ReadHoldingRegister(WORD wAddressHoldReg, WORD *wResult);
  WORD WriteSingleCoil(WORD wAddressCoil, BOOL Coil);
  WORD WriteSingleRegister(WORD wAddressHoldReg, WORD wRegValue);
  void FloatToHex(float Data, WORD *Result);
  float HexToFloat(WORD *Data);
 private:
  WORD wAddress;
  WORD Write(BYTE *wpData,ULONG ulWriteByte);
  WORD Read(BYTE *wpData,WORD *wpResult);
};

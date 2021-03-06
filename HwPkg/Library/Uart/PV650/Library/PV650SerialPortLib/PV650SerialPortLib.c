/*--------------------------------------------------------------------------------------------------------------------------*/
/*!!Warning: This is a key information asset of Huawei Tech Co.,Ltd                                                         */
/*CODEMARK:kOyQZYzjDpyGdBAEC2GaWuVy7vy/wDnq7gJfHBOj2pBXFF9pJtpDLt9sw5WJiMsUkN5d7jr7
aK5J3kmlnl+vpZ4X5IrQg0R6dsKjrHb0BePRXyTmI6pqqZK/VsgQAFF+TLEhrrYdasNpB+ZM
CUefd5IAVhU7iV0D/ALEu5wQxVKdUNpdTwZc0pRRpDeeLSj3NEXoyuTICFLoXooSFiSUDsFR
ed5HqScGgXGDUKeQC7NtQmkVynuihXW74gQtVkob+bYg6TbiNsCDHFys6iIpzg==*/
/*--------------------------------------------------------------------------------------------------------------------------*/
/** @file
  UART Serial Port library functions

  Copyright (c) 2006 - 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Uefi.h>
#include <Library/PcdLib.h>
#include <Library/SerialPortLib.h>
#include <Protocol/SerialIo.h>

#include "PV650SerialPortLib.h"


/**
  Initialize the serial device hardware.
  
  If no initialization is required, then return RETURN_SUCCESS.
  If the serial device was successfuly initialized, then return RETURN_SUCCESS.
  If the serial device could not be initialized, then return RETURN_DEVICE_ERROR.
  
  @retval RETURN_SUCCESS        The serial device was initialized.
  @retval RETURN_DEVICE_ERROR   The serail device could not be initialized.

**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
    UINT32 ulUartClkFreq;
    
    //设置发送和接收数据的长度为8位
    *(volatile UINT8 *)(UART_LCR_REG) = UART_LCR_DLS8;
    
    // 设置FIFO控制寄存器*/
    *(volatile UINT8 *)(UART_FCR_REG) = UART_FCR_EN | UART_FCR_RXCLR | UART_FCR_TXCLR;
    
    // 设置能够波特率的分频因子
    // ULCR.DLAB = 1, Enable UDLB/UDMB/UAFR的访问
    *(volatile UINT8 *)(UART_LCR_REG) = UART_LCR_DLAB | UART_LCR_DLS8;
    
    // uniBIOS-g00179230 2013-5-23, 串口驱动
    // 设置串口频率，20000000
    ulUartClkFreq = PcdGet32(PcdUartClkInHz);

    *(volatile UINT8 *)(UART_DLL_REG) = (ulUartClkFreq / (16 * (UINT32)BAUDRATE) ) & 0xff;
    *(volatile UINT8 *)(UART_DLH_REG) = ((ulUartClkFreq/ (16 * (UINT32)BAUDRATE) ) >> 8 ) & 0xff;
    
    // ULCR.DLAB = 0, 禁止 UDLB/UDMB/UAFR的访问
    *(volatile UINT8 *)(UART_LCR_REG) = UART_LCR_DLS8;
    
    // 设置中断使能寄存器, 禁止DUART中断 LSR
    *(volatile UINT8 *)(UART_IEL_REG) = 0x00;
    
    //start_d00183345_2012-11-7, 去掉很多的S打印
    // 写's' 
    //*(volatile UINT8 *)(UART_THR_REG) = 0x53;
    //end_d00183345_2012-11-7
    
    return RETURN_SUCCESS;
}


/**
  Write data from buffer to serial device. 
 
  Writes NumberOfBytes data bytes from Buffer to the serial device.  
  The number of bytes actually written to the serial device is returned.
  If the return value is less than NumberOfBytes, then the write operation failed.

  If Buffer is NULL, then ASSERT(). 

  If NumberOfBytes is zero, then return 0.

  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the serial device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes written to the serial device.  
                           If this value is less than NumberOfBytes, then the read operation failed.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
)
{
  UINTN  Result;

  if (NULL == Buffer) {
    return 0;
  }

  Result = NumberOfBytes;

  while (NumberOfBytes--) {

    SerialPortWriteChar(*Buffer);
    Buffer++;
  }

  return Result;
}


/**
  Reads data from a serial device into a buffer.

  @param  Buffer           Pointer to the data buffer to store the data read from the serial device.
  @param  NumberOfBytes    Number of bytes to read from the serial device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes read from the serial device.  
                           If this value is less than NumberOfBytes, then the read operation failed.

**/
UINTN
EFIAPI
SerialPortRead (
  OUT UINT8     *Buffer,
  IN  UINTN     NumberOfBytes
)
{
  UINTN  Result;

  if (NULL == Buffer) {
    return 0;
  }

  Result = 0;

  while (NumberOfBytes--) {
    //
    // Wait for the serail port to be ready.
    //
    *Buffer=SerialPortReadChar();
    Buffer++ ;
    Result++;
  }

  return Result;
}

/**
  Polls a serial device to see if there is any data waiting to be read.

  Polls aserial device to see if there is any data waiting to be read.
  If there is data waiting to be read from the serial device, then TRUE is returned.
  If there is no data waiting to be read from the serial device, then FALSE is returned.

  @retval TRUE             Data is waiting to be read from the serial device.
  @retval FALSE            There is no data waiting to be read from the serial device.

**/
BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  )
{
  //接收fifo中有值 
  return (BOOLEAN) ((*(volatile UINT8 *)(UART_LSR_REG) & UART_LSR_DR) == UART_LSR_DR);

}

/*t00216239从1380移植*/
VOID SerialPortWriteChar(UINT8 scShowChar)
{
    UINT32 ulLoop = 0;
      
    while(ulLoop < (UINT32)UART_SEND_DELAY)
    {
        /* 不需要等待THR寄存器为空，只要发送FIFO未满就可以写了-x68638 */
        /* 检查UART_USR[TFNF]，判断发送FIFO未满标志，如果发送FIFO未满，可写入数据 */
        if ((*(volatile UINT8 *)(UART_USR_REG) & 0x02) == 0x02)
        {
            break;
        }
        
        ulLoop++;
    }
    *(volatile UINT8 *)(UART_THR_REG) = (UINT8)scShowChar;

    ulLoop = 0;
    while(ulLoop < (UINT32)UART_SEND_DELAY)   /*等发送缓冲区空*/
    {
        if ((*(volatile UINT8 *)(UART_USR_REG) & 0x04) == 0x04)
        {
            break;
        }
        ulLoop++;
    }
    
    //while ((*(volatile UINT8 *) (UART_LSR_REG) & UART_LSR_TEMT) == 0);//t00216239
           
    return;
}


UINT8 SerialPortReadChar(VOID)
{
    UINT8 recvchar = 0;

    while(1)
    {
        if ((*(volatile UINT8 *)(UART_LSR_REG) & UART_LSR_DR) == UART_LSR_DR)
        {
            break;
        }
    }
    
    recvchar = (*(volatile UINT8 *)(UART_RBR_REG)); 
    
    return recvchar;
}

/**
  Set new attributes to PL011.

  @param  BaudRate                The baud rate of the serial device. If the baud rate is not supported,
                                  the speed will be reduced down to the nearest supported one and the
                                  variable's value will be updated accordingly.
  @param  ReceiveFifoDepth        The number of characters the device will buffer on input. If the specified
                                  value is not supported, the variable's value will be reduced down to the
                                  nearest supported one.
  @param  Timeout                 If applicable, the number of microseconds the device will wait
                                  before timing out a Read or a Write operation.
  @param  Parity                  If applicable, this is the EFI_PARITY_TYPE that is computer or checked
                                  as each character is transmitted or received. If the device does not
                                  support parity, the value is the default parity value.
  @param  DataBits                The number of data bits in each character
  @param  StopBits                If applicable, the EFI_STOP_BITS_TYPE number of stop bits per character.
                                  If the device does not support stop bits, the value is the default stop
                                  bit value.

  @retval EFI_SUCCESS             All attributes were set correctly on the serial device.
  @retval EFI_INVALID_PARAMETERS  One or more of the attributes has an unsupported value.

**/
RETURN_STATUS
EFIAPI
SerialPortSetAttributes (
  IN UINT64              BaudRate,
  IN UINT32              ReceiveFifoDepth,
  IN UINT32              Timeout,
  IN EFI_PARITY_TYPE     Parity,
  IN UINT8               DataBits,
  IN EFI_STOP_BITS_TYPE  StopBits
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Set the serial device control bits.

  @param  Control                 Control bits which are to be set on the serial device.

  @retval EFI_SUCCESS             The new control bits were set on the serial device.
  @retval EFI_UNSUPPORTED         The serial device does not support this operation.
  @retval EFI_DEVICE_ERROR        The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortSetControl (
  IN UINT32                  Control
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Get the serial device control bits.

  @param  Control                 Control signals read from the serial device.

  @retval EFI_SUCCESS             The control bits were read from the serial device.
  @retval EFI_DEVICE_ERROR        The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32                  *Control
  )
{
    
    if (SerialPortPoll ()) {
        // If a character is pending don't set EFI_SERIAL_INPUT_BUFFER_EMPTY
        *Control = EFI_SERIAL_OUTPUT_BUFFER_EMPTY;
    } else {
        *Control = EFI_SERIAL_INPUT_BUFFER_EMPTY | EFI_SERIAL_OUTPUT_BUFFER_EMPTY;
    }
    return EFI_SUCCESS;
  //return EFI_UNSUPPORTED;
}


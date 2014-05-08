/*
*
*/


// serial to i2c protocol
enum serialToI2CProtocol
{
  i2c_write  = 100,    //d
  i2c_read,            //e
  i2c_writeRead,       //f
  i2c_checkDevice,     //g
  i2c_deviceList,      //h
  i2c_resetDevices,    //i
  i2c_error,           //j
  i2c_messageError,    //k
  i2c_noCommand,       //l
  i2c_info,            //m
  i2c_resetI2CFreq,
  i2c_set400kHz,
  i2c_setWriteReadDelay,
  i2c_setResetPin,
  i2c_setResetTime,
  i2c_setResetSettleTime,
  i2c_setInputBufferSize
};



/*
 *******************************************************
 *  protocol documentation
 *******************************************************
 
 
 //------------------------------------
 // i2c_write
 //------------------------------------
 
 writes a number of bytes to i2c slave
 
 [i2c_write, slaveID, numberOfBytes, byte1, byte2, byte3, ..., 13, 10]

 return sequence:
 [i2c_write, slaveID, result, 13, 10]
 
 result: (see Ardunio documentation of Wire.endTransmission)
   0:success
   1:data too long to fit in transmit buffer
   2:received NACK on transmit of addresults
   3:received NACK on transmit of data
   4:other error 
 
 
 write example:
 [i2c_write, 23, 1, 103, 13, 10]
 
 write 1 byte (103) to slave 23
 
 
 
 //------------------------------------
 // i2c_read
 //------------------------------------
 
 read a number of bytes from i2c slave
 
 [i2c_read, slaveID, numberOfBytes, 13, 10]
 
 return sequence:
 [i2c_read, slaveID, readByteCount, byte1, byte2, byte3, ..., 13, 10]
 
 
 read example:
 [i2c_read, 23, 2, 13, 10]
 
 reads 2 bytes from slave with ID 23
  
 
 
 //------------------------------------
 // i2c_writeRead
 //------------------------------------
 
 1. write a number of bytes to i2c slave.
 2. wait for a while (writeReadDelay) to let slave settle
 3. read a number of bytes from i2c slave
 
 [i2c_write_read, slaveID, numberOfBytesToSend, byte1, byte2, byte3, ..., i2c_read, numberOfBytesToRead, 13, 10]
 
 minimal message length: 5
 
 
 return sequence like with i2c_read:
 [i2c_read, slaveID, readByteCount, byte1, byte2, byte3, ..., 13, 10]
 
 
 write_read example:
 [i2c_write_read, 23, 1, byteToI2CSlave, i2c_read, 1, 13, 10]
 
 writes one byte to slave 23 and reads one byte from same slave
 
  
 
 //------------------------------------
 // i2c_checkDevice
 //------------------------------------
 
 check if a certain slave is available
 
 [i2c_checkDevice, slaveID, 13, 10]
 
 return sequence: 
 [i2c_checkDevice, slaveID, result, 13, 10]
 
  result: (see Wire.endTransmission)
   0:success
   1:data too long to fit in transmit buffer
   2:received NACK on transmit of addresults
   3:received NACK on transmit of data
   4:other error 
 
 
 
 check example:
 [i2c_checkDevice, 23, 13, 10]

  checks alve with id 23 

 
 //------------------------------------
 // i2c_deviceList
 //------------------------------------
 
 gets a full devicelist with all slaves
 
 [i2c_deviceList, 13, 10]
  
 return sequence:
 [i2c_deviceList, numberOfDevices, deviceID1, deviceID2, deviceID3, ..., 13, 10]
 
 
 //------------------------------------
 // i2c_resetDevices
 //------------------------------------
 
 pulls resetPin LOW, waits for resetTime [ms] and sets resetPin HIGH.
 resetDevices() is blocking until the sequence is done
 
 
 
 
 //------------------------------------
 // i2c_error
 //------------------------------------
 
 i2c_error is sent if an error occured
 
 
 //------------------------------------
 // i2c_messageError
 //------------------------------------
 
 a message error is sent if the command is not well formed.
 this happens if there is too less arguments.
 
 //------------------------------------
 // i2c_resetI2CFreq,
 //------------------------------------
 reset i2c frequency to default
 
 //------------------------------------
 // i2c_set400kHz
 //------------------------------------
 set i2c frequency to 400kHz
 
 //------------------------------------
 i2c_setWriteReadDelay
 //------------------------------------
 set writeReadDelay
 
 //------------------------------------
 i2c_setResetPin
 //------------------------------------
 set reset pin
 
 //------------------------------------
 i2c_setResetTime
 //------------------------------------
 set resetTime
 
 //------------------------------------
 i2c_setResetSettleTime
 //------------------------------------
 set reset settle time
 
 //------------------------------------
 i2c_setInputBufferSize
 //------------------------------------
 set inputbuffer size
 default: 128
 
 
 *******************************************************
 *  protocol documentation end
 *******************************************************
*/

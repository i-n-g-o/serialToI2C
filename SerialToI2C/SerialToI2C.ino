/*
*  Serial to i2c translator
*
*  written by Ingo Randolf - 2014
*
*  consider this code as public domain.
*/

#include <Wire.h>

#include "SerialToI2C.h"


// some additional debugging info
//#define DO_I2C_INFO


//TODO: set writeReadDelay (use all bytes)
//TODO: resetTime > 255 ms (use all bytes)
//TODO: resetSettleTime > 255 (use all bytes)
//TODO: bigger unputbuffer than 256? (use all bytes)
//TODO: add ability to set start and end condition
//TODO: turn on/off reporting??



// store default TWBR
uint8_t TWBR_default;

//------------------------------------
// delay between write and read commands
// when using i2c_writeRead
unsigned int writeReadDelay = 10000; //in microseconds

//------------------------------------
// reset pin and time
uint8_t resetPin = 4; // pin connected to reset line
unsigned int resetTime = 10; // time to wait between reset signals in [ms]
unsigned int resetSettleTime = 100; // time to wait for devices to settle after reset [ms]

//------------------------------------
// serial input buffer
byte* inputBuffer;
unsigned int inputBufferSize = 128;
unsigned int bufferIdx = 0;

//------------------------------------
// message conditions

// message start condition
byte packetInStart[] = {}; // start condition
byte packetInStartSize;
boolean packetInStartFound = false;

// message end condition
byte packetInEnd[] = {13, 10}; // end condition (breaker)
byte packetInEndSize;

// output start / end condition
byte packetOutStart[] = {};
byte packetOutEnd[] = {13, 10};


//------------------------------------
// setup
//------------------------------------
void setup()
{
  // get default packet condition sizes
  packetInStartSize = sizeof(packetInStart);
  packetInEndSize = sizeof(packetInEnd);
  
  if (packetInStartSize == 0)
  {
    // in case there is no start condition
    packetInStartFound = true;
  }
  
  // create input buffer with default size
  inputBuffer = (byte*)malloc(inputBufferSize);
  resetInputBuffer();
  
  // reset Pin
  pinMode(resetPin, OUTPUT);

  // wait a bit, in case i2c slaves needs to settle
  // needed?
  delay(resetSettleTime);

  // init Wire library
  Wire.begin();
  
  // store default TWBR
  TWBR_default = TWBR;

  // begin serial communication
  Serial.begin(115200);
}



//------------------------------------
// loop
//------------------------------------
void loop()
{
  //---------------------------------------------------------
  // read from serial  
  int incomingByte;

  while (Serial.available() > 0)
  {
    // read data
    incomingByte = Serial.read();

    // test if valid
    if (incomingByte < 0) break;


    // test on index
    if (bufferIdx >= inputBufferSize)
    {
      resetInputBuffer();
    }

    // add incoming byte to input buffer
    inputBuffer[bufferIdx] = (byte)incomingByte;
    // increement buffer index, this indicates the length
    bufferIdx++;

    // check for start and end condition
    // only, if there is enough bytes in buffer
    if (bufferIdx >= (packetInStartSize + packetInEndSize))
    {
      //
      boolean bPacketTest = true;
      byte i;
      
      //------------------------------------
      // test for start condition
      if (packetInStartSize > 0 &&
          !packetInStartFound)
      {
        // search for start condition on current location
        for (i = 1; i <= packetInStartSize; i++)
        {
          if (inputBuffer[bufferIdx - i] != packetInStart[packetInStartSize - i])
          {
            bPacketTest = false;
            break;
          }
        }
        
        if (!bPacketTest)
        {
          // no start condition
          // continue receiving bytes...
          continue;
        }
        
        packetInStartFound = true;
      }
      
      
      //------------------------------------
      // test for end condition
      // only if start was found
      if (packetInStartFound)
      {        
        // reset packet test
        bPacketTest = true;
        for (i = 1; i <= packetInEndSize; i++)
        {
          if (inputBuffer[bufferIdx - i] != packetInEnd[packetInEndSize - i])
          {
            bPacketTest = false;
            break;
          }
        }
        
        
        if (bPacketTest)
        {
          // we got full packet, get real size of message
          int messageLength = bufferIdx - packetInEndSize - packetInStartSize;
          
          if (messageLength > 0)
          {
            // analyze packet, strip start and end
            analyzePacket(inputBuffer+packetInStartSize, messageLength);
          }
          
          // doen with messahe, wipe input buffer
          resetInputBuffer();
          
          continue;
        }
      } // if packetInStartFound
    }
  }

  // wait a bit
  delay(5);

}


void resetInputBuffer()
{
  // wipe input buffer and reset index
  memset(inputBuffer, 0, inputBufferSize);
  bufferIdx = 0;
  
  if (packetInStartSize > 0)
  {
    packetInStartFound = false;
  }
}



//----------------------------------------------------------------------------
// analyze a packet
//----------------------------------------------------------------------------
void analyzePacket(byte* buffer_in, int messageLength)
{
  // safety
  if (buffer_in == 0 || messageLength <= 0) return;
  
  byte slaveAddress = 0;
  int writeByteCount = 0;
  int readByteCount = 0;
  
  byte* byteBuffer = 0;
  byte result;


  // analyze input buffer...
  // command on index 0
  switch ((int)buffer_in[0])
  {
   
    //-----------------------------------------------------------------
  case i2c_resetI2CFreq:
    //-----------------------------------------------------------------
    TWBR = TWBR_default;
    break;
    
    //-----------------------------------------------------------------
  case i2c_set400kHz:
    //-----------------------------------------------------------------
    TWBR = ((F_CPU / 400000L) - 16) / 2; // Set I2C frequency to 400kHz
    break;
    
    //-----------------------------------------------------------------
  case i2c_setWriteReadDelay:
    //-----------------------------------------------------------------
    if (messageLength < 2) 
    {
      Serial.write(i2c_messageError);
      Serial.println();
      break;
    }
    break;
    
    //-----------------------------------------------------------------
  case i2c_setResetPin:
    //-----------------------------------------------------------------
    if (messageLength < 2) 
    {
      Serial.write(i2c_messageError);
      Serial.println();
      break;
    }
    resetPin = buffer_in[1];
    break;
    
    //-----------------------------------------------------------------
  case i2c_setResetTime:
    //-----------------------------------------------------------------
    if (messageLength < 2) 
    {
      Serial.write(i2c_messageError);
      Serial.println();
      break;
    }
    resetTime = buffer_in[1];
    break;
    
    //-----------------------------------------------------------------
  case i2c_setResetSettleTime:
    //-----------------------------------------------------------------
    if (messageLength < 2) 
    {
      Serial.write(i2c_messageError);
      Serial.println();
      break;
    }
    resetSettleTime = buffer_in[1];
    break;
    
    //-----------------------------------------------------------------
  case i2c_setInputBufferSize:
    //-----------------------------------------------------------------
    if (messageLength < 2 ||
        buffer_in[1] == 0) 
    {
      Serial.write(i2c_messageError);
      Serial.println();
      break;
    }
    // set input buffer size
    inputBufferSize = buffer_in[1];
    
    free(inputBuffer);    
    // create input buffer with new size
    inputBuffer = (byte*)malloc(inputBufferSize);
    resetInputBuffer();
  
    break;
    
    
//-----------------------------------------------------------------
//-----------------------------------------------------------------

    //-----------------------------------------------------------------
  case i2c_write:
    //-----------------------------------------------------------------
    if (messageLength < 3) 
    {
      Serial.write(i2c_messageError);
      Serial.println();
      break;
    }

    // buffer_in: [i2c_write, slaveAddress, numberBytes, byte1, ..., 13, 10]
    // eg.: [i2c_write, 23, 1, byte1, 13, 10]
    slaveAddress = buffer_in[1];
    writeByteCount = buffer_in[2];

    if (messageLength >= (3 + writeByteCount))
    {
      //setup byteBuffer        
      byteBuffer = (byte*)malloc(writeByteCount);
      memcpy(byteBuffer, buffer_in+3, writeByteCount);
    }


    if (byteBuffer == 0)
    {
      Serial.write(i2c_error);
      Serial.println();
      break;
    }


    //write bytes to i2c
    result = writeToI2C(slaveAddress, byteBuffer, writeByteCount);

    //send result over serial
    Serial.write(i2c_write);
    Serial.write(slaveAddress);
    Serial.write(result);
    Serial.println();

    break;



    //-----------------------------------------------------------------
  case i2c_read:
    //-----------------------------------------------------------------
    if (messageLength < 3) 
    {
      Serial.write(i2c_messageError);
      Serial.println();
      break;
    }

    // buffer_in: [i2c_read, slaveAddress, bytesToRead, 13, 10]
    // e.g. [i2c_read, 23, 2, 13, 10]

    slaveAddress = buffer_in[1];
    readByteCount = buffer_in[2];

    if (readByteCount > 0)
    {
      // safety
      if (byteBuffer != 0) free(byteBuffer);
      
      // get bytes from device
      // returning bytebuffer is callerowned
      byteBuffer = readFromI2C(slaveAddress, readByteCount);


      // test buffer, send result
      if (byteBuffer != 0)
      {
        Serial.write(i2c_read);
        Serial.write(slaveAddress);
        Serial.write(readByteCount);
        Serial.write((uint8_t*)byteBuffer, (size_t)readByteCount);
        Serial.println(); 
      }
      else
      {
        Serial.write(i2c_error);
        Serial.println();
      }


    }
    else
    {
      // we do not want to read bytes...?
      Serial.write(i2c_read);
      Serial.write(slaveAddress);
      Serial.write((byte)0);
      Serial.println();
    }


    break;


    //-----------------------------------------------------------------
  case i2c_writeRead:
    //-----------------------------------------------------------------
    if (messageLength < 5) 
    {
      Serial.write(i2c_messageError);
      Serial.println();
      break;
    }

    // buffer_in: [i2c_write, slaveAddress, bytecount, byte1, ..., i2c_read, bytecount, 13, 10]
    // e.g. [i2c_write, 23, 1, pika360_mc_getID, i2c_read, 1, 13, 10]
    slaveAddress = buffer_in[1];
    writeByteCount = buffer_in[2];


    if (messageLength >= (5 + writeByteCount))
    {
      if (buffer_in[3+writeByteCount] != i2c_read)
      {
        Serial.write(i2c_messageError);
        Serial.write(buffer_in[3+writeByteCount]);
        Serial.println();
        break;
      }
      
      //setup byteBuffer        
      byteBuffer = (byte*)malloc(writeByteCount);
      memcpy(byteBuffer, buffer_in+3, writeByteCount);   
    }


    if (byteBuffer == 0)
    {
      Serial.write(i2c_messageError);
      Serial.println();
      break;
    }


    readByteCount = buffer_in[3+writeByteCount+1];

    //------------------------------------
    // write bytes to i2c
    result = writeToI2C(slaveAddress, byteBuffer, writeByteCount);

    // test result
    if (result != 0)
    {
      // error occured
      free(byteBuffer);
      
      Serial.write(i2c_messageError);
      Serial.println();
      break;
    }


    if (readByteCount > 0)
    {
      //wait a bit to let slave settle
      delayMicroseconds(writeReadDelay);


      // free buffer
      free(byteBuffer);
      byteBuffer = 0;
      
      //------------------------------------
      // returning bytebuffer is owned by caller
      byteBuffer = readFromI2C(slaveAddress, readByteCount);

      // send result back over serial
      if (byteBuffer != 0)
      {
        Serial.write(i2c_read);
        Serial.write(slaveAddress);
        Serial.write(readByteCount);
        Serial.write((uint8_t*)byteBuffer, (size_t)readByteCount);
        Serial.println();
      }
      else
      {
        Serial.write(i2c_error);
        Serial.println();
      }
    }
    else
    {
      // no read bytes are specified...?
      Serial.write(i2c_read);
      Serial.write(slaveAddress);
      Serial.write((byte)0);
      Serial.println();
    }
    
    break;


    //-----------------------------------------------------------------
  case i2c_checkDevice:
    //-----------------------------------------------------------------
    if (messageLength < 2) 
    {
      Serial.write(i2c_messageError);
      Serial.println();
      break;
    }

    slaveAddress = buffer_in[1];

    result = (int)checkDevices(slaveAddress);

    Serial.write(i2c_checkDevice);
    Serial.write(slaveAddress);
    Serial.write(result);
    Serial.println();

    break;

    //-----------------------------------------------------------------
  case i2c_deviceList:
    //-----------------------------------------------------------------
    scanDevices();
    break;


    //-----------------------------------------------------------------
  case i2c_resetDevices:
    //-----------------------------------------------------------------
    resetDevices();       
    break;

    //-----------------------------------------------------------------
  default:
    //-----------------------------------------------------------------
    Serial.write(i2c_noCommand);
    Serial.println();    
    break;
    
  } // switch command-byte


  // safety
  if (byteBuffer != 0) free(byteBuffer);
}



byte checkDevices(byte address)
{
  byte result;

  Wire.beginTransmission(address);
  result = Wire.endTransmission();

  return result;
}


void scanDevices()
{ 
  byte result;

  //scan devices  
  uint8_t devices[127];  
  size_t deviceCount = 0;


  for (int i=1; i<127; i++)
  {
    Wire.beginTransmission(i);
    result = Wire.endTransmission();

    if (result == 0)
    {
      devices[deviceCount] = i;
      deviceCount++;
    }
  }


  Serial.write(i2c_deviceList);
  Serial.write(deviceCount);
  Serial.write(devices, deviceCount);
  Serial.println();
}




byte writeToI2C(byte address, byte* data, int dataLength)
{
  byte result;

  Wire.beginTransmission(address); // transmit to device #4

  Wire.write(data, dataLength);

  result = Wire.endTransmission();    // stop transmitting

//  result codes
//    0:success
//    1:data too long to fit in transmit buffer
//    2:received NACK on transmit of address
//    3:received NACK on transmit of data
//    4:other error 

  return result;
}




byte* readFromI2C(byte address, int dataLength)
{
  int count = 0;

  
  // BUFFER_LENGTH is defined in Wire.h
  if (dataLength > BUFFER_LENGTH)
  {
    dataLength = BUFFER_LENGTH;
    
    // send info if enabled
#ifdef DO_I2C_INFO
    Serial.write(i2c_info);
    Serial.print("resize datasize to: ");
    Serial.print(BUFFER_LENGTH);
    Serial.println();
#endif
  }
  
  
  // setup buffer  
  byte* byteBuffer = (byte*)malloc(dataLength);
  if (byteBuffer == 0) return 0;

  // buffer to 0
  memset(byteBuffer, 0, dataLength);

  Wire.requestFrom((int)address, dataLength);
  
  while(Wire.available())
  {     
    byteBuffer[count] = Wire.read();    
    count++;
  }


  // test readlength  
  if (count < dataLength)      
  {
    // error???
    free(byteBuffer);
    byteBuffer = 0;
  }
  
  // return bytebuffer or 0 
  return byteBuffer;
}



void resetDevices()
{
  digitalWrite(resetPin, LOW);
  delay(resetTime);
  digitalWrite(resetPin, HIGH);
  delay(resetSettleTime);
}



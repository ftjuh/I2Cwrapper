/*!
   @file simpleBuffer.h
   @brief Simple and ugly serialization buffer for any data type. Just a buffer,
   a current position pointer used for reading from *and* writing to the buffer,
   and template functions for reading and writing any data type. The template
   technique is adapted from Nick Gammon's I2C_Anything library, the CRC8
   routine is from him also.
   @section author Author
   Copyright (c) 2022 juh
   @section license License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
*/

#ifndef SimpleBuffer_h
#define SimpleBuffer_h

// #define DEBUG // uncomment for serial debugging, don't forget Serial.begin() in your setup()


#include <Arduino.h>

#if !defined(log)
#if defined(DEBUG)
#define log(...)       Serial.print(__VA_ARGS__)
#else
#define log(...)
#endif // DEBUG
#endif // log





class SimpleBuffer
{
public:
  /*!
   * @brief Allocate and reset buffer.
   * @param buflen Maximum length of buffer in bytes. Take into account that
   * the first byte [0] is used as CRC8 checksum.
  */
  void init(uint8_t buflen);

  /*!
   * @brief Write any basic data type to the buffer at the current position and
   * increment the position pointer according to the type's size.
   * @param value Data to write.
  */
  template <typename T> void write(const T& value);

  /*!
   * @brief Read any basic data type from the buffer from the current position and
   * increment the position pointer according to the type's size.
   * @param value Variable to read to. Amount of data read depends on size of this
   * type. As reading could fail, you best initialize the variable with some
   * default value.
  */
  template <typename T> void read(T& value);

  /*!
   * @brief Reset the position pointer to the start of the buffer (which is[1]
   * as [0] is the CRC8 chcksum) without changing the buffer contents. Usually,
   * this will be called before writing new data *and* before reading it.
  */
  void reset();


  /*!
   * @brief Calculate CRC8 checksum for the currently used buffer ([1]...[idx-1])
   * and store it in the first byte [0].
   */
  void setCRC8();

  /*!
   * @brief Check for correct CRC8 checksum. First byte [0] holds the checksum,
   * rest of the currently used buffer is checked.
   * @returns true if CRC8 matches the rest of the used buffer.
   */
  bool checkCRC8();


  // I guess a proper class would make the next three private or safeguard them
  // with access methods. Let's keep 'em public for quick'n'dirty direct access.

  /*!
   * @brief The allocated buffer.
  */
  uint8_t* buffer;

  /*!
   * @brief The position pointer. Remember, [0] holds the CRC8 checksum, so for
   * an empty buffer, idx is 1.
  */
  uint8_t idx; // next position to write to / read from

  /*!
  * @brief Maximum length of buffer in bytes. Read and write operations use this
  * to check for sufficient space (no error handling, though).
  */
  uint8_t maxLen;

private:
  uint8_t calculateCRC8 ();
};


template <typename T> void SimpleBuffer::write(const T& value)
{
  if (idx + sizeof (value) <= maxLen) { // enough space to write to?
    memcpy(&buffer[idx], &value, sizeof (value));
    idx += sizeof (value);
  }
}

// T should be initiated before calling this, as it might return unchanged due to if() statement
template <typename T> void SimpleBuffer::read(T& value)
{
  if (idx + sizeof (value) <= maxLen) { // enough space to read from?
    memcpy(&value, &buffer[idx], sizeof (value));
    idx += sizeof (value);
  }
}


#endif

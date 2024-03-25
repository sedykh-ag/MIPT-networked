#pragma once

class Bitstream
{
private:
  char *data;
  size_t dataLength;
  char *writePtr;
  char *readPtr;

public:
  Bitstream(void *data, size_t dataLength) : 
    data((char *)data), dataLength(dataLength), writePtr((char *)data), readPtr((char *)data)
  { }

  Bitstream(const Bitstream &) = delete;

  template<typename T>
  void write(const T &val)
  {
    memcpy(writePtr, &val, sizeof(T));
    writePtr += sizeof(T);
  }

  template<typename T>
  void read(T &val)
  {
    memcpy(&val, readPtr, sizeof(T));
    readPtr += sizeof(T);
  }
};

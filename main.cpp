#include "holoCalibration.h"

int main(int argc,char*argv[])
{
  std::string fileName = "";
  if(argc > 1)
    fileName = argv[1];
  std::cerr << HoloCalibration::getCalibration(fileName) << std::endl;
  return 0;
}

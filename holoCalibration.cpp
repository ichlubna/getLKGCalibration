#include "holoCalibration.h"
#include "json.hpp"
#include <exception>
#include <iostream>
#include <fstream>
#include <libusb-1.0/libusb.h>
#include <vector>

void printError(int e)
{
    const char *es;
    es = libusb_strerror(e);
    std::cerr << es << std::endl;            
}

HoloCalibration::Calibration HoloCalibration::getCalibration()
{
    try
    {
        libusb_context *c; 
        if(libusb_init(&c) < 0) 
            throw std::runtime_error("Cannot Initialize libusb");  
        //libusb_set_debug(c, 3);

        libusb_device **devs;
        ssize_t devCount = libusb_get_device_list(c, &devs);

        if(devCount <= 0)
            throw std::runtime_error("No devices listed");

        int productId{0};
        DeviceType deviceType{NONE};
        int vendorId{0};
        int interfaceId{2};
        for(int i=0; i<devCount; i++)
        {
            struct libusb_device_descriptor desc;
            if(libusb_get_device_descriptor(devs[i], &desc)<0)
                continue;

            if(desc.idVendor == VENDOR_ID_FIRSTGEN)
            {
                deviceType = FIRSTGEN;
                interfaceId = INTERFACE_NUM_FIRSTGEN;
            }
            else if(desc.idVendor == VENDOR_ID_PORTRAIT)
            {
                deviceType = PORTRAIT;
            }

            if(deviceType != NONE)
            {
                vendorId = desc.idVendor;
                productId = desc.idProduct;
                break;
            }
        }
        
        if(productId == 0)
            throw std::runtime_error("No LKG display found");
     
        std::string info;
        if(deviceType == PORTRAIT)
        {
            std::ifstream ifs("/proc/mounts");
            std::string line;
            std::string mountPoint;
            while (getline(ifs, line))
            {
                if (line.find("LKG-") != std::string::npos)
                {
                    int firstSpace = line.find(" ");
                    int secondSpace = line.find(" ", firstSpace + 1);
                    mountPoint = line.substr(firstSpace+1, secondSpace - firstSpace-1);
                }
            }
            std::ifstream ifsCal(mountPoint+"/LKG_calibration/visual.json");
            std::stringstream ss;
            ss << ifsCal.rdbuf();
            info = ss.str();
        }
        else if(deviceType == FIRSTGEN)
        {
            struct libusb_device_handle *dh = NULL; 
            dh = libusb_open_device_with_vid_pid(NULL,vendorId,productId);
            
            if(!dh)
                throw std::runtime_error("Cannot connect to device"); 
      
            libusb_detach_kernel_driver(dh,interfaceId);
                
            if(libusb_claim_interface(dh,interfaceId) < 0 )
                throw std::runtime_error("Cannot claim interface");

            uint8_t bmReqType = 0x80;
            uint8_t bReq = 6;
            uint16_t wVal = 0x0302; 
            uint16_t wIndex = 0x0409;
            unsigned char data[255] = {0};
            std::vector<unsigned char> buffer;
            unsigned int to = 0;
            libusb_control_transfer(dh,bmReqType,bReq,0x0303,wIndex,data,1026,to);
            libusb_control_transfer(dh,bmReqType,bReq,0x0301,wIndex,data,1026,to);
            libusb_control_transfer(dh,bmReqType,bReq,wVal,wIndex,data,255,to);

            buffer.resize(BUFFER_SIZE);
            std::fill(buffer.begin(), buffer.end(), 0);

            int len = 0;
            for(int i=0; i<PACKET_NUM; i++)
            {
                int r;
                std::fill(buffer.begin(), buffer.end(), 0);
                buffer[2] = i; 
                r = libusb_control_transfer(dh,0x21,9,0x300,0x02,buffer.data(),INTERRUPT_SIZE,to);
                if(r<0)
                    throw std::runtime_error("Cannot send control");

                std::fill(buffer.begin(), buffer.end(), 0);
                r = libusb_interrupt_transfer(dh, 0x84, buffer.data(), buffer.size(), &len, 0);
                info.insert(info.end() ,buffer.begin(), buffer.begin()+INTERRUPT_SIZE+1); 
                if(r<0)
                    throw std::runtime_error("Cannot interrupt");
            }
            info = info.substr(info.find("{"), info.find("}}")-info.find("{")+2);
            //std::cerr << info << std::endl;

            for(int i=0; i<7; i++) 
                info.erase(remove(info.begin(), info.end(), i), info.end());
 
            libusb_close(dh);
        }

        libusb_exit(NULL);
  
        auto j = nlohmann::json::parse(info.c_str());

        return {static_cast<float>(j["pitch"]["value"]),
                static_cast<float>(j["slope"]["value"]),
                static_cast<float>(j["center"]["value"]),
                static_cast<float>(j["viewCone"]["value"]),
                static_cast<float>(j["invView"]["value"]),
                static_cast<float>(j["verticalAngle"]["value"]),
                static_cast<float>(j["DPI"]["value"]),
                static_cast<float>(j["screenW"]["value"]),
                static_cast<float>(j["screenH"]["value"]),
                static_cast<float>(j["flipImageX"]["value"]),
                static_cast<float>(j["flipImageY"]["value"]),
                static_cast<float>(j["flipSubp"]["value"]),
                }; 
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << "Returning default calibration for small LKG" << std::endl;
    }
    
    return {};
}

#include "main.h"

// Vars
pjpeg_image_info_t imageInfo;
RESPONSE response;
uint8_t mode[MAX_MODE_LEN];
int mcu_x = 0;
int mcu_y = 0;

void main(){

    // Init LCD
    LCD_Init();

    // Clear the screen
    LCD_Clear(LCD_COLOR_WHITE);
    
    // Init camera uart
    initCameraUart();

    // Set baudrate to 115200
    mode[0] = MCU_UART;
    mode[1] = B_115200_H;
    mode[2] = B_115200_L;
    response = setBaudRate(mode);
    if(response.status != EXEC_COMMAND_RIGHT)
        for(;;); // Error  

    // Set the resolution by means of a downsize option. Zoom out height and width by 1/2
    /*mode[0] = 5;
    response = setDownsize(mode);
    if(response.status != EXEC_COMMAND_RIGHT)
        for(;;); // Error*/


    for(;;){

        // Stop a current frame
        mode[0] = STOP_CURRENT_FRAME;     
        response = fbufControl(mode);
        if(response.status != EXEC_COMMAND_RIGHT)
            for(;;); // Error

        // Init JPEG decoder
        if(pjpeg_decode_init(&imageInfo, readPictureBytes, NULL, 0) != 0)
            for(;;); // Error

        // Draw image    
        while(pjpeg_decode_mcu() != PJPG_NO_MORE_BLOCKS){

            for (int y = 0; y < imageInfo.m_MCUHeight; y += 8)
            {
                const int by_limit = MIN(8, imageInfo.m_height - (mcu_y * imageInfo.m_MCUHeight + y));
                for (int x = 0; x < imageInfo.m_MCUWidth; x += 8)
                {
 
                    unsigned int src_ofs = (x * 8U) + (y * 16U);
                    const unsigned char *pSrcR = imageInfo.m_pMCUBufR + src_ofs;
                    const unsigned char *pSrcG = imageInfo.m_pMCUBufG + src_ofs;
                    const unsigned char *pSrcB = imageInfo.m_pMCUBufB + src_ofs;
                    const int bx_limit = MIN(8, imageInfo.m_width - (mcu_x * imageInfo.m_MCUWidth + x));

                    for (int by = 0; by < by_limit; by++)
                    {
                        for (int bx = 0; bx < bx_limit; bx++)
                        {
   
                            Pixel(mcu_y*imageInfo.m_MCUHeight+y+by,mcu_x*imageInfo.m_MCUWidth+x+bx, ASSEMBLE_RGB(*pSrcR++, *pSrcG++, *pSrcB++));
                        }
 
                        pSrcR += (8 - bx_limit);
                        pSrcG += (8 - bx_limit);
                        pSrcB += (8 - bx_limit);

                    }
                }
            }
 
        mcu_x++;
        if (mcu_x == imageInfo.m_MCUSPerRow)
        {
            mcu_x = 0;
            mcu_y++;
        }

    }
    
    mcu_x = mcu_y = 0;

    // Clear the screen
    LCD_Clear(LCD_COLOR_WHITE);

    // Resume a frame
    mode[0] = RESUME_FRAME;     
    response = fbufControl(mode);
    if(response.status != EXEC_COMMAND_RIGHT)
        for(;;); // Error

    }

}

unsigned char readPictureBytes(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data){

    static uint32_t picLen;
    static uint32_t picAddr;
    uint16_t bytesToRead;

    if(picLen == 0){
        // Reset picAddr
        picAddr = 0;
        // Get a picture length
        mode[0] = CURRENT_FRAME;
        response = getFbufLen(mode);
        if(response.status != EXEC_COMMAND_RIGHT)
            for(;;); // Error
        picLen = *response.responsePtr++;
        picLen <<= 8;
        picLen |= *response.responsePtr++;
        picLen <<= 8;
        picLen |= *response.responsePtr++;
        picLen <<= 8;
        picLen |= *response.responsePtr++;
    }

    // Read a picture data
    mode[0] = CURRENT_FRAME;
    mode[1] = DMA_DATA_TRANSFER;
    if(buf_size < 8)
        bytesToRead = buf_size;
    else
        bytesToRead = MIN((buf_size) - (buf_size % 8), picLen);
    mode[2] = picAddr >> 24;mode[3] = (picAddr >> 16) & 0xFF;
    mode[4] = (picAddr >> 8) & 0xFF; mode[5] = picAddr & 0xFF;
    mode[6] = bytesToRead >> 24;mode[7] = (bytesToRead >> 16) & 0xFF;
    mode[8] = (bytesToRead >> 8) & 0xFF; mode[9] = bytesToRead & 0xFF;
    mode[10] = (DELAY_001MS >> 8);mode[11] = DELAY_001MS & 0xFF;
    readFbuf(mode);
    if(response.status != EXEC_COMMAND_RIGHT)
        for(;;); // Error
    // Read N bytes of a picture data
    readNBytes(pBuf, bytesToRead);
    // Read last five bytes which correspond to response(dummy read)
    readNBytes(NULL, 5);
    
    picLen -= bytesToRead;
    picAddr +=bytesToRead;
    // Number of bytes actually read
    *pBytes_actually_read = bytesToRead;

    return 0;

}



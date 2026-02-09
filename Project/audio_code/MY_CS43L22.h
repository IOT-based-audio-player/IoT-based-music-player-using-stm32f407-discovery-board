
//(1): Header files
#include "stm32f4xx_hal.h"
#include "stdbool.h"

//(2): List of all the defines
#define POWER_CONTROL1					0x02
#define POWER_CONTROL2					0x04
#define CLOCKING_CONTROL 	  		0x05
#define INTERFACE_CONTROL1			0x06
#define INTERFACE_CONTROL2			0x07
#define PASSTHROUGH_A						0x08
#define PASSTHROUGH_B						0x09
#define MISCELLANEOUS_CONTRLS		0x0E
#define PLAYBACK_CONTROL				0x0F
#define PASSTHROUGH_VOLUME_A		0x14
#define PASSTHROUGH_VOLUME_B		0x15
#define PCM_VOLUME_A						0x1A
#define PCM_VOLUME_B						0x1B
#define CONFIG_00								0x00
#define CONFIG_47								0x47
#define CONFIG_32								0x32

#define   CS43L22_REG_MASTER_A_VOL        0x20
#define   CS43L22_REG_MASTER_B_VOL        0x21
#define   CS43L22_REG_HEADPHONE_A_VOL     0x22
#define   CS43L22_REG_HEADPHONE_B_VOL     0x23

#define DAC_I2C_ADDR 			0x94

#define CS43_MUTE				 	0x00

#define CS43_RIGHT				0x01
#define CS43_LEFT				 	0x02
#define CS43_RIGHT_LEFT	 	0x03

#define VOLUME_MASTER(volume)       (((volume) >= 231 && (volume) <=255)? (volume-231) : (volume+25))

//1. Mode Select Enum
typedef enum
{
	MODE_I2S = 0,
	MODE_analog,
}CS43_MODE;


//(3): List of the functions prototypes
void CS43_Init(I2C_HandleTypeDef i2c_handle, CS43_MODE outputMode);
void CS43_Enable_RightLeft(uint8_t side);
void CS43_SetVolume(uint8_t volume);
void CS43_SetMute(bool mute);
void CS43_Start(void);
void CS43_Stop(void);

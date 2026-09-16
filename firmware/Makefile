# 화재·가스 감지 자동 대피 유도 시스템 - 빌드 설정
# 3단계: 개발 환경 구축

MCU = atmega128
F_CPU = 16000000UL
CC = avr-gcc
OBJCOPY = avr-objcopy
TARGET = main

CFLAGS = -Wall -Os -mmcu=$(MCU) -DF_CPU=$(F_CPU)

all: $(TARGET).hex

$(TARGET).elf: $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET).elf $(TARGET).c

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $(TARGET).elf $(TARGET).hex

clean:
	rm -f $(TARGET).elf $(TARGET).hex

.PHONY: all clean

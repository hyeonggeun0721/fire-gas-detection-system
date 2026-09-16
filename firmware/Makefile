# 화재·가스 감지 자동 대피 유도 시스템 - 빌드 설정
# 3단계: 개발 환경 구축
# 5단계: 소스 다중 파일 구성(main.c + debounce.c) 및 호스트용 단위 테스트 타깃 추가

MCU = atmega128
F_CPU = 16000000UL
CC = avr-gcc
OBJCOPY = avr-objcopy
TARGET = main
SRCS = main.c debounce.c

CFLAGS = -Wall -Os -mmcu=$(MCU) -DF_CPU=$(F_CPU)

HOST_CC = cc
HOST_CFLAGS = -Wall -Wextra -std=c99

all: $(TARGET).hex

$(TARGET).elf: $(SRCS) debounce.h
	$(CC) $(CFLAGS) -o $(TARGET).elf $(SRCS)

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $(TARGET).elf $(TARGET).hex

# 하드웨어/시뮬레이터 없이 로직만 검증하는 호스트 단위 테스트
test:
	$(HOST_CC) $(HOST_CFLAGS) -o tests/test_debounce tests/test_debounce.c debounce.c
	./tests/test_debounce

clean:
	rm -f $(TARGET).elf $(TARGET).hex tests/test_debounce

.PHONY: all clean test

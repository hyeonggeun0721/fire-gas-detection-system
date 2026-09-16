# 화재·가스 감지 자동 대피 유도 시스템 - 빌드 설정
# 3단계: 개발 환경 구축
# 5단계: 소스 다중 파일 구성(main.c + debounce.c) 및 호스트용 단위 테스트 타깃 추가
# 7단계: ADC/온도 변환 모듈(adc.c, temp.c) 추가
# 8단계: 가스 센서 변환 모듈(gas.c) 추가
# 9단계: 인터럽트 플래그 처리 패턴 호스트 테스트 추가

MCU = atmega128
F_CPU = 16000000UL
CC = avr-gcc
OBJCOPY = avr-objcopy
TARGET = main
SRCS = main.c debounce.c lcd.c adc.c temp.c gas.c

CFLAGS = -Wall -Os -mmcu=$(MCU) -DF_CPU=$(F_CPU)

HOST_CC = cc
HOST_CFLAGS = -Wall -Wextra -std=c99

all: $(TARGET).hex

$(TARGET).elf: $(SRCS) debounce.h lcd.h adc.h temp.h gas.h
	$(CC) $(CFLAGS) -o $(TARGET).elf $(SRCS)

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $(TARGET).elf $(TARGET).hex

# 하드웨어/시뮬레이터 없이 로직만 검증하는 호스트 단위 테스트
test:
	$(HOST_CC) $(HOST_CFLAGS) -o tests/test_debounce tests/test_debounce.c debounce.c
	./tests/test_debounce
	$(HOST_CC) $(HOST_CFLAGS) -o tests/test_temp tests/test_temp.c temp.c
	./tests/test_temp
	$(HOST_CC) $(HOST_CFLAGS) -o tests/test_interrupt_flag tests/test_interrupt_flag.c
	./tests/test_interrupt_flag

clean:
	rm -f $(TARGET).elf $(TARGET).hex tests/test_debounce tests/test_temp tests/test_interrupt_flag

.PHONY: all clean test

# 3주차 보고서 — 개발 환경 구축

## 이번 주 목표

- AVR-GCC 프로젝트 스켈레톤(`firmware/main.c`) 및 Makefile 작성
- 로컬 빌드 환경(avr-gcc, avrdude) 구축
- 개발 환경 설정 과정에서 흔히 발생하는 실수를 의도적으로 재현하고 해결

## 구현 내용

- 로컬에 AVR-GCC 9.5.0(osx-cross/avr tap, Homebrew) 및 avrdude 8.3을 설치했다.
- `firmware/main.c`: 아직 센서/출력 로직이 없는 최소 스켈레톤. `_delay_ms()`만 호출하는
  무한 루프로, 4단계 이후 GPIO 제어 코드가 이 위에 추가될 예정이다.
- `firmware/Makefile`: `avr-gcc -mmcu=$(MCU) -DF_CPU=$(F_CPU)`로 컴파일하고
  `avr-objcopy`로 `.hex`를 생성하는 최소 빌드 규칙을 작성했다. `F_CPU`는
  2단계에서 가정한 16MHz 외부 크리스탈 기준으로 설정했다(실제 프리스케일러/타이머
  계산은 10단계에서 이 값을 기준으로 진행).
- 저장소에 `.gitignore`를 추가해 빌드 산출물(`*.elf`, `*.hex`)과 `.DS_Store`가
  커밋되지 않도록 했다.

## 발생한 오류 및 트러블슈팅 과정

**증상**: `make` 실행 시 컴파일이 시작조차 되지 않고 아래 오류로 즉시 종료됨.

```
avr-gcc -Wall -Os -mmcu=atmega18 -DF_CPU=16000000UL -o main.elf main.c
avr-gcc: fatal error: cannot read spec file 'device-specs/specs-atmega18': No such file or directory
compilation terminated.
make: *** [main.elf] Error 1
```

**원인 분석**: Makefile의 `MCU` 변수에 목표 칩인 `atmega128`을 `atmega18`로 오타 입력했다.
`avr-gcc`는 `-mmcu`로 지정된 이름에 해당하는 `device-specs/specs-<MCU>` 파일을 찾아
레지스터 맵·메모리 크기 등 아키텍처별 스펙을 로드하는데, `atmega18`은 실재하지 않는
가상의 부품명이라 스펙 파일 자체를 찾지 못해 전처리/컴파일 단계 이전에 즉시 실패했다.
이는 튜토리얼의 Makefile을 급하게 베껴 쓰다가 자릿수를 빼먹는 전형적인 오타 실수다.

**해결 과정**:
1. 오류 메시지의 `specs-atmega18`이라는 문자열에서 MCU 이름이 잘못 지정되었음을 특정했다.
2. 2단계 아키텍처 문서(`docs/architecture.md`)에서 목표 칩이 ATmega128임을 재확인했다.
3. Makefile의 `MCU = atmega18`을 `MCU = atmega128`로 수정했다.
4. `make clean && make`로 재빌드하여 `main.elf`, `main.hex`가 정상 생성되는 것을 확인했다.
5. `avr-size main.elf`로 `text=188 bytes`가 출력되어 정상적으로 ATmega128 타깃으로
   빌드되었음을 확인했다.

## 시뮬레이션 검증 결과

이번 주 코드는 아직 GPIO 등 관측 가능한 출력이 없어(다음 주부터 LED 제어 추가),
시뮬레이터에서 확인할 내용은 "빌드된 `.hex`가 ATmega128 프로젝트에 정상적으로
로드되고 클럭이 멈추지 않고 동작하는지" 정도로 제한된다.

**확인해야 할 시뮬레이션 시나리오** (Proteus/SimulIDE에서 직접 확인 후 결과 공유 요청):
1. Proteus/SimulIDE에서 ATmega128 부품을 배치하고 클럭 소스를 16MHz로 설정한다.
2. `firmware/main.hex`를 프로그램 메모리로 로드한다.
3. 시뮬레이션을 실행하고, MCU가 리셋을 반복하거나 정지하지 않고 계속 동작(런타임 에러 없음)하는지 확인한다.
4. (선택) 디버거의 프로그램 카운터가 `main` 함수의 `while(1)` 루프 구간을 반복적으로 순회하는지 확인한다.

> 위 시나리오는 아직 사용자 확인 전입니다. 직접 실행하신 결과를 알려주시면 이 절을 업데이트하겠습니다.

## 다음 주 계획

- 4단계: GPIO 기초 — LED/부저 출력 제어
  - `main.c`에 LED/부저용 GPIO 초기화 및 제어 코드 추가
  - 의도적 오류: 풀업저항/극성 미설정으로 인한 오동작 재현 및 수정

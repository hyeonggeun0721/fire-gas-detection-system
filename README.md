# 화재·가스 감지 자동 대피 유도 시스템

ATmega128 마이크로프로세서 텀 프로젝트 (2인 1조)

## 프로젝트 개요

- **플랫폼**: ATmega128 (AVR-GCC 툴체인)
- **시뮬레이터**: SimulIDE 1.1.0-SR2 (`simulation/fire_gas_system.sim1`)
- **기능**: 온도/가스/불꽃 센서로 화재를 감지 → 부저 경보 → 서보모터로 비상구 자동 개방
  → LCD에 대피 경로 표시 → UART로 PC(관제실)에 신고 메시지 전송
  → 관제실은 한 글자 명령(S 조회, R 원격 복구, T 화재 시험, O/C 비상구)으로 원격 조작
- **제출 마감**: 11월 30일 (발표 후 제출)

## 바로 실행해 보기

1. [SimulIDE](https://simulide.com/p/downloads/) 1.1.0을 설치한다.
2. SimulIDE에서 `simulation/fire_gas_system.sim1`을 연다.
3. 전원 버튼을 누르면 `firmware/main.hex`가 자동으로 올라가고 LCD에 `STATUS: NORMAL`이 표시된다.
4. 가변저항(Temp/Gas)과 버튼(Flame/Test-Reset)으로 상태를 바꿔 본다. 조작법은 `simulation/README.md`.

펌웨어를 고쳤다면 `firmware`에서 `make`로 다시 빌드한 뒤 시뮬레이션 전원을 껐다 켠다.

## 주요 문서

| 문서 | 내용 |
|---|---|
| `docs/final-report.md` | 최종 보고서 (흐름도, 레지스터 설정 근거, 시뮬레이션 결과) |
| `docs/test-scenarios.md` | 시나리오 A~H와 실행 결과 |
| `docs/troubleshooting-simulide.md` | 시뮬레이션에서 겪은 문제 6가지와 해결 과정 |
| `docs/requirements.md`, `docs/architecture.md` | 요구사항, 핀맵·회로 설계 |
| `reports/week01.md` ~ `week15.md` | 주차별 개발 보고서 |

## 저장소 구조

```
fire-gas-detection-system/
  firmware/           # C 소스코드 (AVR-GCC), main.hex, tests/ (호스트 단위 테스트)
  simulation/         # SimulIDE 회로 파일
  docs/               # 보고서, 요구사항, 핀맵, 시나리오, 트러블슈팅, 캡처
  reports/            # 주차별 보고서 (week01.md ~ week15.md)
  README.md
```

## 진행 방식

프로젝트는 15단계(주차)로 나누어 순차적으로 개발한다. 각 단계는 다음 순서로 진행한다.

1. 기능 구현 (코드 작성)
2. 의도적 오류 삽입 및 커밋 (초보 개발자가 흔히 저지르는 실수 재현)
3. 오류 증상 재현 및 트러블슈팅 기록 (증상 → 원인 분석 → 해결 과정)
4. 오류 수정 및 커밋
5. 해당 주차 보고서 작성 (`reports/week##.md`)
6. 커밋 및 푸시

### 커밋 메시지 컨벤션

- `[Week01] 기능: GPIO LED 제어 구현`
- `[Week01] 버그: 풀업저항 미설정으로 인한 오동작 재현`
- `[Week01] 수정: DDRx/PORTx 설정 수정 및 오류 해결`
- `[Week01] 문서: 1주차 보고서 작성`
- 시뮬레이션 검증 단계는 `[Sim]` 접두어를 쓴다.

## 브랜치 전략

`main` 단일 브랜치로 진행한다.

## 15단계 커리큘럼

1. 프로젝트 초기 설정 및 요구사항 정의
2. 시스템 아키텍처 및 회로 설계 초안
3. 개발 환경 구축
4. GPIO 기초 — LED/부저 출력 제어
5. 키패드 또는 테스트 버튼 입력 및 디바운싱
6. LCD 디스플레이 연동
7. ADC 온도 센서 읽기
8. ADC 가스 센서 읽기 및 다중 채널 전환
9. 불꽃 센서(디지털 입력) 및 외부 인터럽트 처리
10. 타이머/카운터 기반 주기적 센싱 스케줄링
11. 서보모터 PWM 제어 — 비상구 자동 개방
12. 화재 판단 알고리즘 및 상태 기계 설계
13. UART 통신 — PC 관제실로 신고 메시지 전송
14. 통합 테스트 및 시나리오별 시뮬레이션 검증
15. 최종 통합, 성능/코드 최적화, 최종 보고서 및 발표자료 정리

## 참고

- 실제 하드웨어 없이 SimulIDE와 simavr(타이밍 측정), 호스트 단위 테스트(`make test`)로 검증했다.
- SimulIDE의 ATmega128 모델은 복귀 주소를 3바이트로 처리하므로, 빌드 결과에 `rcall .+0`이 없어야 한다
  (`avr-objdump -d firmware/main.elf | grep -c "rcall\s*\.+0"` → 0). 자세한 내용은 `docs/troubleshooting-simulide.md`.

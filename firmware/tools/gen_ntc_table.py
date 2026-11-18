"""temp.c의 ntc_raw_table을 다시 계산하는 스크립트.
서미스터나 분압 저항 값을 바꾸면 이 값을 고친 뒤 실행해 출력된 표를 temp.c에 붙여 넣는다."""
import math

B, R25, R_FIXED = 3950, 10000, 10000   # NTC B값, 25°C 저항, 아래쪽 고정 저항
T_MIN, T_MAX, STEP = -20, 120, 5

def adc_raw(t_c):
    r_ntc = R25 * math.exp(B * (1 / (t_c + 273.15) - 1 / 298.15))
    return round(1023 * R_FIXED / (R_FIXED + r_ntc))

table = [adc_raw(t) for t in range(T_MIN, T_MAX + 1, STEP)]
for i in range(0, len(table), 8):
    print("    " + ", ".join(f"{x:4d}" for x in table[i:i + 8]) + ",")

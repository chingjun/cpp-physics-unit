#include <stdio.h>
#include "physics_unit.h"

int main() {
    printf("day(1) = %f seconds\n", Day(1).value<Second>());
    printf("second(86400) = %f minutes\n", DSecond(86400).value<Minute>());
    printf("second(86400) / day(1) = %f\n", (DSecond(86400) / Day(1)).value<Dimensionless>());
    printf("meter(1) / day(1) = %f m/s = %f km/h\n", (Meter(1) / Day(5)).value<MeterPerSecond>(), (Meter(1) / Day(5)).value<KilometerPerHour>());
    printf("3.5 km/h = %.*f mph\n", 3, KilometerPerHour(3.5).value<MilePerHour>());
    printf("3.5 m/s * 10 hour = %f mile\n", (MeterPerSecond(3.5) * Hour(10)).value<Mile>());
    return 0;
}

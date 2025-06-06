#include "calculator_ops.h"
#include <stdio.h> // For snprintf
#include <string.h> // For strcpy

CalcResult add(double a, double b) {
    CalcResult res;
    res.value = a + b;
    strcpy(res.error, ""); // No error
    return res;
}

CalcResult subtract(double a, double b) {
    CalcResult res;
    res.value = a - b;
    strcpy(res.error, ""); // No error
    return res;
}

CalcResult multiply(double a, double b) {
    CalcResult res;
    res.value = a * b;
    strcpy(res.error, ""); // No error
    return res;
}

CalcResult divide(double a, double b) {
    CalcResult res;
    if (b == 0) {
        res.value = 0; // Or some indicator like NaN, but error string is primary
        snprintf(res.error, sizeof(res.error), "Error: Division by zero!");
    } else {
        res.value = a / b;
        strcpy(res.error, ""); // No error
    }
    return res;
}

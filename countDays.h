#ifndef COUNTDAYS_H
#define COUNTDAYS_H

#include <string>
//Leet 1360. Number of Days Between Two Dates
int daysInMonth(int month, int year) {
    if (month == 2) {
        return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 29 : 28;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    }
    return 31;
}

int countDays(std::string date) {
    int year = std::stoi(date.substr(0, 4));
    int month = std::stoi(date.substr(5, 2));
    int day = std::stoi(date.substr(8, 2));
    
    int days = 0;
    
    // Add days for complete years
    for (int y = 1971; y < year; y++) {
        days += (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365;
    }
    
    // Add days for complete months
    for (int m = 1; m < month; m++) {
        days += daysInMonth(m, year);
    }
    
    // Add remaining days
    days += day;
    
    return days;
}


int daysBetweenDates(std::string date1, std::string date2) {
    return abs(countDays(date1) - countDays(date2));
}

#endif

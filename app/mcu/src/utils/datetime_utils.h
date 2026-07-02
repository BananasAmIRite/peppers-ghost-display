#pragma once

#include <string>

std::string dayOfWeekToShortString(int dayIndex) {
  switch (dayIndex) {
    case 1: return "Sun";
    case 2: return "Mon";
    case 3: return "Tue";
    case 4: return "Wed";
    case 5: return "Thu";
    case 6: return "Fri";
    case 7: return "Sat";
    default: return "---";
  }
}

std::string dayOfWeekToString(int dayIndex) {
  switch (dayIndex) {
    case 1: return "Sunday";
    case 2: return "Monday";
    case 3: return "Tuesday";
    case 4: return "Wednesday";
    case 5: return "Thursday";
    case 6: return "Friday";
    case 7: return "Saturday";
    default: return "Unknown"; // Error handling for invalid numbers
  }
}



std::string monthToString(int monthIndex) {
  switch (monthIndex) {
    case 1:  return "January";
    case 2:  return "February";
    case 3:  return "March";
    case 4:  return "April";
    case 5:  return "May";
    case 6:  return "June";
    case 7:  return "July";
    case 8:  return "August";
    case 9:  return "September";
    case 10: return "October";
    case 11: return "November";
    case 12: return "December";
    default: return "Unknown"; // Error handling for invalid numbers
  }
}

std::string monthToShortString(int monthIndex) {
  switch (monthIndex) {
    case 1:  return "Jan";
    case 2:  return "Feb";
    case 3:  return "Mar";
    case 4:  return "Apr";
    case 5:  return "May";
    case 6:  return "Jun";
    case 7:  return "Jul";
    case 8:  return "Aug";
    case 9:  return "Sep";
    case 10: return "Oct";
    case 11: return "Nov";
    case 12: return "Dec";
    default: return "---";
  }
}

std::string timeTo12hStringNoSeconds(int h, int m) {
  std::string period = " AM";
  
  // Determine AM or PM
  if (h >= 12) {
    period = " PM";
  }
  
  // Convert hour from 0-23 to 1-12
  if (h == 0) {
    h = 12; // Midnight
  } else if (h > 12) {
    h = h - 12;
  }
  
  std::string timeStr = "";
  if (h < 10) timeStr += "0";
  timeStr += std::to_string(h) + ":";
  
  if (m < 10) timeStr += "0";
  timeStr += std::to_string(m) + period;
  
  return timeStr; // Returns "02:30 PM"
}
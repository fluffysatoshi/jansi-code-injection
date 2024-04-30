#ifndef JANSI_UTIL_H
#define JANSI_UTIL_H

typedef enum { JANSI_LCK, JANSI_SO, JANSI_NONE } Jansi_File_Type;
Jansi_File_Type jansi_file_type(const char *);
int cp(int to_fd, const char *from);

#endif

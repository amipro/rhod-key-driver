/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
 
#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))
#define MAX(x,y)	(x>y?x:y)
#define TEST_BIT(x,addr) (1UL & (addr[x/8] >> (x & 0xff)))

// configuration file data
extern char *conf_file;
extern int lineno;
#define conf_error(fmt,...) error("Error %s:%d " fmt, conf_file, lineno, ##__VA_ARGS__)

void set_debug(int level);
void debug(const char *s,...);
void syserror(const char *s,...);
void error(const char *s,...);
int load_configuration(char *path);
void setconsole(int console);

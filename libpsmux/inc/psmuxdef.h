#ifndef	__PS_MUX_DEF_H__
#define __PS_MUX_DEF_H__


typedef int    gint;
typedef signed char gint8;
typedef unsigned char guint8;
typedef signed short gint16;
typedef unsigned short guint16;
typedef signed int gint32;
typedef unsigned int guint32;
typedef signed long long gint64;
typedef unsigned long long guint64;
typedef gint   gboolean;

typedef unsigned char   guchar;
typedef unsigned short  gushort;
typedef unsigned long   gulong;
typedef unsigned int    guint;

#define g_critical printf
#define g_warning printf

#define MUX_ERROR   (1)
#define MEM_ERROR   (2)
#define MUX_OK      (0)

#define INVALID_TS  (-1)

#define INVALID_STREAM_INDEX  (-1)

#ifndef NULL
	#define NULL    (0)
#endif

#endif

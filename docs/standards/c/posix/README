reserved identifiers in posix 2008

format
------

name, header, type, options


identifier types
----------------

a	any kind
i	filescope identifier
d	macro definition
f	function name
x	extern object name
t	typedef name
s	struct tag
u	union tag
e	enum tag
m	struct or union member
c	enum member (const)
dc	integer symbolic constant
xd	macro or extern object
fd	function and/or macro
ti	incomplete typedef type name
si	incomplete struct type tag
ui	incomplete union type tag

macro parameters do not have namespace issues, jump labels and function or
object identifiers with internal linkage (static) are covered by i and a
types (the standard does not specify such identifiers), the i type includes
t,s,u,e,m,c,ti,si,ui type identifiers as well

f type identifiers can always have a macro implementation as well

fd type identifiers are like f except they may be only implemented as a macro

dc type identifiers are macros that may be defined as enum constants as well

symbol types that may have external linkage:
a,f,x,xd,fd

symbol types that must have external linkage:
f,x

symbol types that may be macros:
a,d,f,dc,xd,fd

symbol types that must be macros:
d,dc


visibility constraints
----------------------

p	defined identifier
r	reserved identifier or identifier pattern
w	weak identifier

defined identifiers are made visible according to the identifier type when the
header is included

reserved identifiers may be provided by the implementation if the header is
included

weak identifiers are not defined in the header but they have special meaning
(for d type it means the header behaviour is changed if it is defined before
inclusion, for x type it means extern linkage symbol that the application can
override)


options
-------

option names and option test macros in unistd.h:
ADV	_POSIX_ADVISORY_INFO
CPT	_POSIX_CPUTIME
FSC	_POSIX_FSYNC
IP6	_POSIX_IPV6
MSG	_POSIX_MESSAGE_PASSING
MON	_POSIX_MONOTONIC_CLOCK
PIO	_POSIX_PRIORITIZED_IO
PS	_POSIX_PRIORITY_SCHEDULING
RS	_POSIX_RAW_SOCKETS
SHM	_POSIX_SHARED_MEMORY_OBJECTS
SPN	_POSIX_SPAWN
SS	_POSIX_SPORADIC_SERVER
SIO	_POSIX_SYNCHRONIZED_IO
TCT	_POSIX_THREAD_CPUTIME
TSA	_POSIX_THREAD_ATTR_STACKADDR
TSS	_POSIX_THREAD_ATTR_STACKSIZE
TPI	_POSIX_THREAD_PRIO_INHERIT
TPP	_POSIX_THREAD_PRIO_PROTECT
TPS	_POSIX_THREAD_PRIORITY_SCHEDULING
TSH	_POSIX_THREAD_PROCESS_SHARED
TSP	_POSIX_THREAD_SPORADIC_SERVER
RPI	_POSIX_THREAD_ROBUST_PRIO_INHERIT
RPP	_POSIX_THREAD_ROBUST_PRIO_PROTECT
TRC	_POSIX_TRACE
TEF	_POSIX_TRACE_EVENT_FILTER
TYM	_POSIX_TYPED_MEMORY_OBJECTS
XSR	_XOPEN_STREAMS
ML	_POSIX_MEMLOCK
MLR	_POSIX_MEMLOCK_RANGE
CD	_POSIX2_C_DEV
FD	_POSIX2_FORT_DEV
FR	_POSIX2_FORT_RUN
BE	_POSIX2_PBS
SD	_POSIX2_SW_DEV
UP	_POSIX2_UPE
UU	_XOPEN_UUCP
MX	__STDC_IEC_559__
CX	posix
XSI	_XOPEN_UNIX
OB	obsolete

options that imply other options:
XSR	OB
TEF	OB
TRC	OB
all but MX or OB	CX

feature test macros the application can define:
_POSIX_C_SOURCE	request everything except XSI and non-supported options
_XOPEN_SOURCE	request everything except non-supported options

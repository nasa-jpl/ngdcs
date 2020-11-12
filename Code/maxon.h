/* Copyright 2013, by the California Institute of Technology.  ALL RIGHTS
   RESERVED.  United States Government Sponsorship acknowledged.  Any 
   commercial use must be negotiated with the Office of Technology Transfer
   at the California Institute of Technology.

   This software may be subject to U.S. export control laws.  By accepting
   this software, the user agrees to comply with all applicable U.S. export
   laws and regulations.  User has the responsibility to obtain export 
   licenses, or other export authority as may be required before exporting
   such information to foreign countries or providing access to foreign
   persons.

   Please do not redistribute this file separate from the NGDCS source
   distribution.  For questions regarding this software, please contact the
   author, Alan Mazer, alan.s.mazer@jpl.nasa.gov */


extern int Maxon_init_device(char *error_msg);
extern int Maxon_home(char *error_msg);
extern int Maxon_position(int pos, char *error_msg);
extern int Maxon_close_device(char *error_msg);

#define SHUTTER_CLOSED_POS          -630000
#define SHUTTER_OPEN_POS            -25000 /* prevent chatter against hard stop */

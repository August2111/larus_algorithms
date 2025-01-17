/***********************************************************************//**
 * @file		NMEA_format.cpp
 * @brief		ASCII converters for NMEA string output
 * @author		Dr. Klaus Schaefer
 * @copyright 		Copyright 2021 Dr. Klaus Schaefer. All rights reserved.
 * @license 		This project is released under the GNU Public License GPL-3.0

    <Larus Flight Sensor Firmware>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 **************************************************************************/

#include "NMEA_format.h"
#include "ascii_support.h"
#include "embedded_math.h"

#define USE_MWV		1
#define USE_PLARD	0

#define ANGLE_SCALE 1e-7f
#define MPS_TO_NMPH 1.944f // 90 * 60 NM / 10000km * 3600 s/h
#define RAD_TO_DEGREE_10 572.958f
#define RAD_TO_DEGREE 57.2958f
#define METER_TO_FEET 3.2808f
#define MPS_TO_KMPH 3.6f

ROM char HEX[]="0123456789ABCDEF";

//! format an integer into ASCII with exactly two digits after the decimal point
//! @param number value * 100
char * integer_to_ascii_2_decimals( int32_t number, char *s)
{
  if( number < 0)
    {
      *s++='-';
      number = -number;
    }
  if( number >= 1000)
    {
      s = format_integer( number / 1000, s);
      number %= 1000;
    }
  *s++=HEX[number / 100]; // format 1 digit plus exactly 2 decimals
  *s++='.';
  number %= 100;
  *s++=HEX[number / 10];
  *s++=HEX[number % 10];
  *s=0;
  return s;
}

//! format an integer into ASCII with one decimal
//! @param number value * 10
char * integer_to_ascii_1_decimal( int32_t number, char *s)
{
  if( number < 0)
    {
      *s++='-';
      number = -number;
    }
  if( number >= 100)
    {
      s = format_integer( number / 100, s);
      number %= 100;
    }
  *s++=HEX[number / 10]; // format exactly 1 decimal
  *s++='.';
  *s++=HEX[number % 10];
  *s=0;
  return s;
}

//! append an angle in ASCII into a NMEA string
char * angle_format ( double angle, char * p, char posc, char negc)
{
  bool pos = angle > 0.0f;
  if (!pos)
    angle = -angle;

  int degree = (int) angle;

  *p++ = degree / 10 + '0';
  *p++ = degree % 10 + '0';

  double minutes = (angle - (double) degree) * 60.0;
  int min = (int) minutes;
  *p++ = min / 10 + '0';
  *p++ = min % 10 + '0';

  *p++ = '.';

  minutes -= min;
  minutes *= 100000;
  min = (int) (minutes + 0.5f);

  p[4] = min % 10 + '0';
  min /= 10;
  p[3] = min % 10 + '0';
  min /= 10;
  p[2] = min % 10 + '0';
  min /= 10;
  p[1] = min % 10 + '0';
  min /= 10;
  p[0] = min % 10 + '0';

  p += 5;

  *p++ = ',';
  *p++ = pos ? posc : negc;
  return p;
}

char * format_GNSS_timestamp(const coordinates_t &coordinates, char *p)
{
  unsigned hundredth_seconds;
  if( coordinates.nano < 0)
      hundredth_seconds=0; // just ignore any (small) negative idfference
  else
      hundredth_seconds=coordinates.nano / 10000000;

//  assert( hundredth_seconds < 100);

  *p++ = (coordinates.hour)   / 10 + '0';
  *p++ = (coordinates.hour)   % 10 + '0';
  *p++ = (coordinates.minute) / 10 + '0';
  *p++ = (coordinates.minute) % 10 + '0';
  *p++ = (coordinates.second) / 10 + '0';
  *p++ = (coordinates.second) % 10 + '0';
  *p++ = '.';
  *p++ = hundredth_seconds / 10 +'0';
  *p++ = hundredth_seconds % 10 +'0';
  *p++ = ',';

  return p;
}

ROM char GPRMC[]="$GPRMC,";

//! NMEA-format time, position, groundspeed, track data
void format_RMC (const coordinates_t &coordinates, char *p)
{
  p = append_string( p, GPRMC);
  p = format_GNSS_timestamp( coordinates, p);

  *p++ = coordinates.sat_fix_type != 0 ? 'A' : 'V';
  *p++ = ',';

  p = angle_format (coordinates.latitude, p, 'N', 'S');
  *p++ = ',';

  p = angle_format (coordinates.longitude, p, 'E', 'W');
  *p++ = ',';

  float value = coordinates.speed_motion * MPS_TO_NMPH;

  unsigned knots = (unsigned)(value * 10.0f + 0.5f);
  *p++ = knots / 1000 + '0';
  knots %= 1000;
  *p++ = knots / 100 + '0';
  knots %= 100;
  *p++ = knots / 10 + '0';
  *p++ = '.';
  *p++ = knots % 10 + '0';
  *p++ = ',';

  float true_track = coordinates.heading_motion;
  if( true_track < 0.0f)
    true_track += 6.2832f;
  int angle_10 = true_track * 10.0 + 0.5;

  *p++ = angle_10 / 1000 + '0';
  angle_10 %= 1000;
  *p++ = angle_10 / 100 + '0';
  angle_10 %= 100;
  *p++ = angle_10 / 10 + '0';
  *p++ = '.';
  *p++ = angle_10 % 10 + '0';

  *p++ = ',';

  *p++ = (coordinates.day) / 10 + '0';
  *p++ = (coordinates.day) % 10 + '0';
  *p++ = (coordinates.month) / 10 + '0';
  *p++ = (coordinates.month) % 10 + '0';
  *p++ = ((coordinates.year)%100) / 10 + '0';
  *p++ = ((coordinates.year)%100) % 10 + '0';

  p=append_string( p, ",,,A");
  *p = 0;
}

ROM char GPGGA[]="$GPGGA,";

//! NMEA-format position report, sat number and GEO separation
char *format_GGA( const coordinates_t &coordinates, char *p)
{
  p = append_string( p, GPGGA);
  p = format_GNSS_timestamp( coordinates, p);

  p = angle_format (coordinates.latitude, p, 'N', 'S');
  *p++ = ',';

  p = angle_format (coordinates.longitude, p, 'E', 'W');
  *p++ = ',';

  *p++ = coordinates.sat_fix_type  >= 0 ? '1' : '0';
  *p++ = ',';

  *p++ = (coordinates.SATS_number) / 10 + '0';
  *p++ = (coordinates.SATS_number) % 10 + '0';
  *p++ = ',';

  *p++ = '1'; // fake HDOP
  *p++ = '.';
  *p++ = '0';
  *p++ = ',';

  uint32_t altitude_msl_dm = coordinates.position.e[DOWN] * -10.0f;
  p = integer_to_ascii_1_decimal( altitude_msl_dm, p);
  *p++ = ',';
  *p++ = 'M';
  *p++ = ',';

  int32_t geo_sep_10 = coordinates.geo_sep_dm;
  p = integer_to_ascii_1_decimal( geo_sep_10, p);
  *p++ = ',';
  *p++ = 'M';
  *p++ = ','; // no DGPS
  *p++ = ',';
  *p++ = 0;

  return p;
}

ROM char GPMWV[]="$GPMWV,";

//! format wind reporting, standard  NMEA sequence
char *format_MWV ( float wind_north, float wind_east, char *p)
{
  p = append_string( p, GPMWV);

//  wind_north = 3.0; // this setting reports 18km/h from 53 degrees
//  wind_east = 4.0;

  while (*p)
    ++p;

  float direction = ATAN2( -wind_east, -wind_north);
  if( direction < 0.0f)
    direction += 360.0f;
  int angle_10 = round( direction * RAD_TO_DEGREE_10);
  p=integer_to_ascii_1_decimal( angle_10, p);
  *p++ = ',';
  *p++ = 'T'; // true direction
  *p++ = ',';

  float value = SQRT( SQR( wind_north) + SQR( wind_east));

  unsigned wind_10 = value * 10.0f;
  p=integer_to_ascii_1_decimal( wind_10, p);
  *p++ = ',';
  *p++ = 'M'; // m/s
  *p++ = ',';
  *p++ = 'A'; // valid
  *p++ = 0;

  return p;
}

#if USE_MWV
ROM char HCHDT[] = "$HCHDT,";

//! create HCHDM sentence to report true heading
void format_HCHDT( float true_heading, char *p) // report magnetic heading
{
  int heading = round(true_heading * 573.0f); // -> 1/10 degree
  if( heading < 0)
    heading += 3600;

  p = append_string( p, HCHDT);
  p = integer_to_ascii_1_decimal( heading, p);
  *p++ = ',';
  *p++ = 'T';

  *p = 0;
}
#endif  // USE_MWV

// ********* Larus-specific protocols *************************************
#if USE_LARUS_NMEA_EXTENSIONS

ROM char PLARB[]="$PLARB,";

void format_PLARB ( float voltage, char *p)
{
    p = append_string( p, PLARB);
    p = integer_to_ascii_2_decimals( round( voltage * 100.0f), p);
    *p = 0;
}

ROM char PLARA[]="$PLARA,";

void format_PLARA ( float roll, float nick, float yaw, char *p)
{
    p = append_string( p, PLARA);

    p = integer_to_ascii_1_decimal( round(roll * RAD_TO_DEGREE_10), p);
    *p++ = ',';

    p = integer_to_ascii_1_decimal( round(nick * RAD_TO_DEGREE_10), p);
    *p++ = ',';

    if( yaw < 0.0f)
        yaw += 6.2832f;
    p = integer_to_ascii_1_decimal(round(yaw * RAD_TO_DEGREE_10), p);

    *p = 0;
}

#if USE_PLARD
ROM char PLARD[] = "$PLARD,";

void format_PLARD(float air_density, char *p) {
  p = append_string(p, PLARD);
  // Instant air density in g/m^3.
  p = integer_to_ascii_1_decimal(round(air_density * 10000.0f), p);
  p = append_string(p, ",A"); // always report "valid" for the moment
  *p = 0;
}
#endif  // USE_PLARD

ROM char PLARV[] = "$PLARV,";

char *format_PLARV(float vario, float avg_vario, float altitude, float TAS,
                   char *p) {
  vario = CLIP(vario, -10.0f, 10.0f);
  avg_vario = CLIP(avg_vario, -10.0f, 10.0f);

  p = append_string(p, PLARV);

  p = integer_to_ascii_1_decimal(round(vario * 10.0f), p); // in m/s
  *p++ = ',';

  p = integer_to_ascii_1_decimal(round(avg_vario * 10.0f), p); // in m/s
  *p++ = ',';

  p = format_integer(altitude, p); // in m
  *p++ = ',';

  p = integer_to_ascii_1_decimal(round(TAS * MPS_TO_KMPH * 10.0f), p); // in m/s
  *p = 0;

  return p;
}

ROM char PLARW[]="$PLARW,";

//! format wind reporting NMEA sequence
void format_PLARW ( float wind_north, float wind_east, char windtype, char *p)
{
  p = append_string( p, PLARW);

    //wind_north = 3.0; // this setting reports 18km/h from 53 degrees
    //wind_east = 4.0;

    // report WHERE the wind the comes from, instead of our wind speed vector, so negative sign
    float direction = ATAN2( -wind_east, -wind_north);

    // map to 0..359 degrees
    int angle = round( direction * RAD_TO_DEGREE);
    if( angle < 0)
        angle += 360;
    p=format_integer( angle, p);
    *p++ = ',';

    int speed = round( MPS_TO_KMPH * SQRT( SQR( wind_north) + SQR( wind_east)));
    p=format_integer( speed, p);
    *p++ = ',';

    *p++ = windtype;

    p = append_string( p, ",A"); // always report "valid" for the moment
    *p=0;
}
#endif

// ********* Generic stuff ************************************************
inline char hex4( uint8_t data)
{
  return HEX[data];
}

//! test a line for valid NMEA checksum
bool NMEA_checksum( const char *line)
 {
 	uint8_t checksum = 0;
 	if( line[0]!='$')
 		return false;
 	const char * p;
 	for( p=line+1; *p && *p !='*'; ++p)
 		checksum ^= *p;
 	return ( (p[0] == '*') && hex4( checksum >> 4) == p[1]) && ( hex4( checksum & 0x0f) == p[2]) && (p[3] == 0);
 }

//! add end delimiter, evaluate and add checksum and add CR+LF
char * NMEA_append_tail( char *p)
 {
 	uint8_t checksum = 0;
 	if( p[0]!='$')
 		return 0;
 	for( p=p+1; *p && *p !='*'; ++p)
 		checksum ^= *p;
 	p[0] = '*';
 	p[1] = hex4(checksum >> 4);
 	p[2] = hex4(checksum & 0x0f);
 	p[3] = '\r';
 	p[4] = '\n';
 	p[5] = 0;
 	return p+5;
 }

//! this procedure formats all our NMEA sequences
void format_NMEA_string( const output_data_t &output_data, string_buffer_t &NMEA_buf)
{
  char *next = NMEA_buf.string;

  // NMEA-format time, position, groundspeed, track data
  format_RMC ( output_data.c, next);
  next = NMEA_append_tail ( next);

  // NMEA-format position report, sat number and GEO separation
  format_GGA ( output_data.c, next);
  next = NMEA_append_tail (next);

#if USE_MWV
  // report wind, the standard way, redundant to PLARW
  format_MWV (output_data.wind_average.e[NORTH], output_data.wind_average.e[EAST], next);
  next = NMEA_append_tail (next);

#endif

  format_HCHDT( output_data.euler.y, next);
  next = NMEA_append_tail(next);

#if USE_LARUS_NMEA_EXTENSIONS
  // instant wind
  format_PLARW (output_data.wind.e[NORTH], output_data.wind.e[EAST], 'I', next);
  next = NMEA_append_tail (next);

  // average wind
  format_PLARW (output_data.wind_average.e[NORTH], output_data.wind_average.e[EAST], 'A', next);
  next = NMEA_append_tail (next);

  // aircraft attitude
  format_PLARA(output_data.euler.r, output_data.euler.n, output_data.euler.y, next);
  next = NMEA_append_tail(next);

  // battery_voltage
  format_PLARB( output_data.m.supply_voltage, next);
  next = NMEA_append_tail(next);

#if USE_PLARD
  // air_density
  format_PLARD(output_data.air_density, next);
  next = NMEA_append_tail(next);
#endif // USE_PLARD

  // report instant and average total-energy-compensated variometer, pressure
  // altitude, TAS
  format_PLARV(output_data.vario, output_data.integrator_vario,
               output_data.pressure_altitude, output_data.TAS, next);
  next = NMEA_append_tail(next);

#endif

//  assert(   next - NMEA_buf.string < string_buffer_t::BUFLEN);
  NMEA_buf.length = next - NMEA_buf.string;
}


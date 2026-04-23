#include "../common.h"

void
CG_AdjustFrom640 (float *x, float *y, float *w, float *h)
{
  *x *= gscreenXScale;
  *y *= gscreenYScale;
  *w *= gscreenXScale;
  *h *= gscreenYScale;
}

void
CG_DrawPic (float x, float y, float width, float height, qhandle_t hShader)
{
  CG_AdjustFrom640 (&x, &y, &width, &height);
  trap_R_DrawStretchPic (x, y, width, height, 0, 0, 1, 1, NULL, hShader);
}

/*
================
CG_DrawRotatePic

Coordinates are 640*480 virtual values
A width of 0 will draw with the original image width
rotates around the upper right corner of the passed in point
=================
*/
void
CG_DrawRotatePic (float x, float y, float width, float height, float angle,
                  qhandle_t hShader)
{
  CG_AdjustFrom640 (&x, &y, &width, &height);
  trap_R_DrawRotatePic (x, y, width, height, 0, 0, 1, 1, angle, hShader);
}

/*
================
CG_DrawRotatePic2

Coordinates are 640*480 virtual values
A width of 0 will draw with the original image width
Actually rotates around the center point of the passed in coordinates
=================
*/
void
CG_DrawRotatePic2 (float x, float y, float width, float height, float angle,
                   qhandle_t hShader)
{
  CG_AdjustFrom640 (&x, &y, &width, &height);
  trap_R_DrawRotatePic2 (x, y, width, height, 0, 0, 1, 1, angle, hShader);
}


qboolean
CG_WorldCoordToScreenCoordFloat (vec3_t worldCoord, float *x, float *y)
{
  int xcenter, ycenter;
  vec3_t local, transformed;
  vec3_t vfwd;
  vec3_t vright;
  vec3_t vup;
  float xzi;
  float yzi;

  xcenter = refdef->width / 2;  //gives screen coords adjusted for resolution
  ycenter = refdef->height / 2; //gives screen coords adjusted for resolution

  //NOTE: did it this way because most draw functions expect virtual 640x480 coords
  //      and adjust them for current resolution
  //xcenter = 640 / 2;//gives screen coords in virtual 640x480, to be adjusted when drawn
  //ycenter = 480 / 2;//gives screen coords in virtual 640x480, to be adjusted when drawn

  AngleVectors (refdef->viewangles, vfwd, vright, vup);

  VectorSubtract (worldCoord, refdef->vieworg, local);

  transformed[0] = DotProduct (local, vright);
  transformed[1] = DotProduct (local, vup);
  transformed[2] = DotProduct (local, vfwd);

  // Make sure Z is not negative.
  if (transformed[2] < 0.01)
    {
      return qfalse;
    }

  xzi = xcenter / transformed[2] * (90.0 / refdef->fov_x);
  yzi = ycenter / transformed[2] * (90.0 / refdef->fov_y);

  *x = xcenter + xzi * transformed[0];
  *y = ycenter - yzi * transformed[1];

  return qtrue;
}

qboolean
CG_WorldCoordToScreenCoord (vec3_t worldCoord, int *x, int *y)
{
  float xF, yF;
  qboolean retVal = CG_WorldCoordToScreenCoordFloat (worldCoord, &xF, &yF);
  *x = (int) xF;
  *y = (int) yF;
  return retVal;
}

char *QDECL
va (char *format, ...)
{
  va_list argptr;
  static char string[2][32000]; // in case va is called by nested functions
  static int index = 0;
  char *buf;

  buf = string[index & 1];
  index++;

  va_start (argptr, format);
  vsprintf (buf, format, argptr);
  va_end (argptr);

  return buf;
}

void
vectoangles (const vec3_t value1, vec3_t angles)
{
  float forward;
  float yaw, pitch;

  if (value1[1] == 0 && value1[0] == 0)
    {
      yaw = 0;
      if (value1[2] > 0)
        {
          pitch = 90;
        }
      else
        {
          pitch = 270;
        }
    }
  else
    {
      if (value1[0])
        {
          yaw = (atan2 (value1[1], value1[0]) * 180 / M_PI);
        }
      else if (value1[1] > 0)
        {
          yaw = 90;
        }
      else
        {
          yaw = 270;
        }
      if (yaw < 0)
        {
          yaw += 360;
        }

      forward = sqrt (value1[0] * value1[0] + value1[1] * value1[1]);
      pitch = (atan2 (value1[2], forward) * 180 / M_PI);
      if (pitch < 0)
        {
          pitch += 360;
        }
    }

  angles[PITCH] = -pitch;
  angles[YAW] = yaw;
  angles[ROLL] = 0;
}

void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
  float angle;
  static float sr, sp, sy, cr, cp, cy;
  // static to help MS compiler fp bugs

  angle = angles[YAW] * (M_PI * 2 / 360);
  sy = sin (angle);
  cy = cos (angle);
  angle = angles[PITCH] * (M_PI * 2 / 360);
  sp = sin (angle);
  cp = cos (angle);
  angle = angles[ROLL] * (M_PI * 2 / 360);
  sr = sin (angle);
  cr = cos (angle);

  if (forward)
    {
      forward[0] = cp * cy;
      forward[1] = cp * sy;
      forward[2] = -sp;
    }
  if (right)
    {
      right[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
      right[1] = (-1 * sr * sp * sy + -1 * cr * cy);
      right[2] = -1 * sr * cp;
    }
  if (up)
    {
      up[0] = (cr * sp * cy + -sr * -sy);
      up[1] = (cr * sp * sy + -sr * cy);
      up[2] = cr * cp;
    }
}

float
AngleMod (float a)
{
  a = (360.0 / 65536) * ((int) (a * (65536 / 360.0)) & 65535);
  return a;
}

float
AngleDelta (float angle1, float angle2)
{
  return AngleNormalize180 (angle1 - angle2);
}

vec_t
VectorNormalize (vec3_t v)
{
  float length, ilength;

  length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
  length = sqrt (length);

  if (length)
    {
      ilength = 1 / length;
      v[0] *= ilength;
      v[1] *= ilength;
      v[2] *= ilength;
    }

  return length;
}

float
AngleNormalize180 (float angle)
{
  angle = AngleNormalize360 (angle);
  if (angle > 180.0)
    {
      angle -= 360.0;
    }
  return angle;
}

float
AngleNormalize360 (float angle)
{
  return (360.0 / 65536) * ((int) (angle * (65536 / 360.0)) & 65535);
}

char *
Info_ValueForKey (const char *s, const char *key)
{
  char pkey[BIG_INFO_KEY];
  static char value[2][BIG_INFO_VALUE]; // use two buffers so compares
  // work without stomping on each other
  static int valueindex = 0;
  char *o;

  if (!s || !key || strlen (s) >= BIG_INFO_STRING)
    return "";

  valueindex ^= 1;
  if (*s == '\\')
    s++;
  while (1)
    {
      o = pkey;
      while (*s != '\\')
        {
          if (!*s)
            return "";
          *o++ = *s++;
        }
      *o = 0;
      s++;

      o = value[valueindex];

      while (*s != '\\' && *s)
        {
          *o++ = *s++;
        }
      *o = 0;

      if (!stricmp (key, pkey))
        return value[valueindex];

      if (!*s)
        break;
      s++;
    }

  return "";
}

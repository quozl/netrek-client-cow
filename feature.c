
#include "config.h"
/* Feature.c
 * 
 * March, 1994.    Joe Rumsey, Tedd Hadley
 * 
 * most of the functions needed to handle SP_FEATURE/CP_FEATURE packets.  fill
 * in the features list below for your client, and add a call to
 * reportFeatures just before the RSA response is sent. handleFeature should
 * just call checkFeature, which will search the list and set the appropriate
 * variable.  features unknown to the server are set to the desired value for
 * client features, and off for server/client features.
 * 
 * feature packets look like:
 *
 * $Log: feature.c,v $
 * Revision 1.4  2006/05/20 08:48:16  quozl
 * fix some valgrind use of uninitialised data reports
 *
 * Revision 1.3  1999/06/11 16:14:17  siegl
 * cambot replay patches
 *
 * Revision 1.2  1999/03/25 20:56:26  siegl
 * CygWin32 autoconfig fixes
 *
 * Revision 1.1.1.1  1998/11/01 17:24:09  siegl
 * COW 3.0 initial revision
 * */

#ifdef nodef
struct feature_cpacket
  {
    char    type;
    char    feature_type;
    char    arg1, arg2;
    int     value;
    char    name[80];
  };

#endif

/* type is CP_FEATURE, which is 60.  feature_spacket is identical. */


#ifdef FEATURE_PACKETS
#include "copyright.h"

#include <stdio.h>
#include <sys/types.h>
#include INC_NETINET_IN
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"


/* not the actual packets: this holds a list of features to report for this
 * client. */
struct feature
  {
    char   *name;
    int    *var;				 /* holds allowed/enabled * * 
						  * status */
    char    feature_type;			 /* 'S' or 'C' for * *
						  * server/client */
    int     value;				 /* desired status */
    char   *arg1, *arg2;			 /* where to copy args, if *
						  * * non-null */
  };

static int _dummy;
void    reportFeatures(void);

struct feature features[] =
{
/* also sent seperately, put here for checking later. should be ok that it's
 * sent twice. */
  {"FEATURE_PACKETS", &F_feature_packets, 'S', 1, 0, 0},

#ifdef nodef
  {"VIEW_BOX", &allowViewBox, 'C', 1, 0, 0},
  {"SHOW_ALL_TRACTORS", &allowShowAllTractorPressor, 'S', 1, 0, 0},
#endif

#ifdef MOTION_MOUSE
  {"CONTINUOUS_MOUSE", &motion_mouse_enablable, 'C', 1, 0, 0},
  {"CONTINUOUS_STEER", &motion_mouse_steering, 'C', 0, 0, 0},
#endif

  {"NEWMACRO", &UseNewMacro, 'C', 1, 0, 0},
  {"SMARTMACRO", &UseSmartMacro, 'C', 1, 0, 0},
  {"WHY_DEAD", &why_dead, 'S', 1, 0, 0},
  {"RC_DISTRESS", &gen_distress, 'S', 1, 0, 0},

#ifdef MULTILINE_MACROS
  {"MULTIMACROS", &multiline_enabled, 'S', 1, 0, 0},
#endif

  {"SBHOURS", &SBhours, 'S', 1, 0, 0},
  {"CLOAK_MAXWARP", &F_cloak_maxwarp, 'S', 1, 0, 0},
  {"SELF_8FLAGS", &F_self_8flags, 'S', 1, 0, 0},
  {"SELF_8FLAGS2", &F_self_8flags2, 'S', 0, 0, 0},
  {"SHIP_CAP", &F_ship_cap, 'S', 1, 0, 0},

#ifdef WARP_DEAD
  {"DEAD_WARP", &F_dead_warp, 'S', 1, 0, 0},
#endif

#ifdef BEEPLITE
  {"BEEPLITE", &_dummy, 'C', 1, &F_beeplite_flags, 0},
#endif

#ifdef HAVE_XPM
  {"AGRI_PIXMAP", &F_agri_pix, 'C', 1, 0, 0},
#endif

#ifdef HAVE_XPM
  {"AGRI_PIXMAP", &F_agri_pix, 'C', 1, 0, 0},
#endif

#ifdef RECORDGAME
  {"MANY_SELF", &F_many_self, 'S', 0, 0, 0},
#endif

#ifdef RECORDGAME
  {"MANY_SELF", &F_many_self, 'S', 0, 0, 0},
#endif

  {0, 0, 0, 0, 0, 0}
};


void
        checkFeature(struct feature_cpacket *packet)
{
  int     i;
  int     value = (int) ntohl(packet->value);
  char    buf[100];

#ifdef DEBUG
  if (packet->type != SP_FEATURE)
    {
      printf("Packet type %d sent to checkFeature!\n", packet->type);
      return;
    }
#endif

  sprintf(buf, "%s: %s(%d)", &packet->name[0],
	  ((value == 1) ? "ON" : (value == 0) ? "OFF" : "UNKNOWN"),
	  value);

#ifdef TOOLS
  W_WriteText(toolsWin, 0, 0, textColor, buf, strlen(buf), W_RegularFont);
#else
  printf("%s\n", buf);
#endif

  for (i = 0; features[i].name != 0; i++)
    {
      if (strcmpi(packet->name, features[i].name) == 0)
	{
	  /* if server returns unknown, set to off for server mods, desired * 
	   * 
	   * * value for client mods. Otherwise,  set to value from server. */
	  *features[i].var = (value == -1 ?
		 (features[i].feature_type == 'S' ? 0 : features[i].value) :
			      value);
	  if (features[i].arg1)
	    *features[i].arg1 = packet->arg1;
	  if (features[i].arg2)
	    *features[i].arg2 = packet->arg2;
	  break;
	}
    }
  if (features[i].name == 0)
    {
      printf("Feature %s from server unknown to client!\n", packet->name);
    }
  /* special cases: */
  if (strcmpi(packet->name, "FEATURE_PACKETS") == 0)
    reportFeatures();

#ifdef HAVE_XPM
  if (strcmpi(packet->name, "FEATURE_PACKETS") == 0)
    {
      if (value == -1)				 /* Features Unknown ... turn 
						  * 
						  * * off */
	F_agri_pix = 0;				 /* AGRI pixmaps just in case 
						  * 
						  */
    }
#endif

  if ((strcmpi(packet->name, "RC_DISTRESS") == 0) && gen_distress)
    distmacro = dist_prefered;

#ifdef BEEPLITE
  if ((strcmpi(packet->name, "BEEPLITE") == 0))
    {
      switch (value)
	{
	case -1:				 /* Unknown, we can use all * 
						  * 
						  * * of the features! */

#ifdef STABLE
	  /* Stable release is absolutely non borgish */
	  F_beeplite_flags =
	      LITE_SOUNDS |
	      LITE_TTS;
#else
	  F_beeplite_flags = LITE_PLAYERS_MAP |
	      LITE_PLAYERS_LOCAL |
	      LITE_SELF |
	      LITE_PLANETS |
	      LITE_SOUNDS |
	      LITE_COLOR |
	      LITE_TTS;
#endif

	  break;
	case 1:
	  if (F_beeplite_flags == 0)
	    {					 /* Server says we can have * 
						  * 
						  * * beeplite, but no * *
						  * options??? must be * *
						  * configured wrong. */
	      F_beeplite_flags = LITE_PLAYERS_MAP |
		  LITE_PLAYERS_LOCAL |
		  LITE_SELF |
		  LITE_PLANETS |
		  LITE_SOUNDS |
		  LITE_COLOR |
		  LITE_TTS;
	    }
	  strcpy(buf, "  disabled:");
	  if (!(F_beeplite_flags & LITE_PLAYERS_MAP))
	    strcat(buf, " LITE_PLAYERS_MAP");
	  if (!(F_beeplite_flags & LITE_PLAYERS_LOCAL))
	    strcat(buf, " LITE_PLAYERS_LOCAL");
	  if (!(F_beeplite_flags & LITE_SELF))
	    strcat(buf, " LITE_SELF");
	  if (!(F_beeplite_flags & LITE_PLANETS))
	    strcat(buf, " LITE_PLANETS");
	  if (!(F_beeplite_flags & LITE_SOUNDS))
	    strcat(buf, " LITE_SOUNDS\n");
	  if (!(F_beeplite_flags & LITE_COLOR))
	    strcat(buf, " LITE_COLOR");
	  if (!(F_beeplite_flags & LITE_TTS))
	    strcat(buf, " LITE_TTS");

	  if (strcmp(buf, "  disabled:"))
	    {

#ifdef TOOLS
	      W_WriteText(toolsWin, 0, 0, textColor, buf, strlen(buf), W_RegularFont);
#else
	      printf("%s\n", buf);
#endif
	    }
	  break;
	case 0:
	  F_beeplite_flags = 0;
	  break;
	default:
	  break;
	}
    }
#endif /* BEEPLITE */
}

sendFeature(char *name, char feature_type, int value, char arg1, char arg2)
{
  struct feature_cpacket packet;

  bzero(&packet, sizeof(packet));
  STRNCPY(packet.name, name, sizeof(packet.name));
  packet.type = CP_FEATURE;
  packet.name[sizeof(packet.name) - 1] = 0;
  packet.feature_type = feature_type;
  packet.value = htonl(value);
  packet.arg1 = arg1;
  packet.arg2 = arg2;
  sendServerPacket((struct player_spacket *) &packet);
}


/* call this from handleRSAKey, before sending the response. */
void
        reportFeatures(void)
{
  struct feature *f;

  for (f = features; f->name != 0; f++)
    {
      if (strcmpi(f->name, "FEATURE_PACKETS") != 0)
	sendFeature(f->name,
		    f->feature_type,
		    f->value,
		    (f->arg1 ? *f->arg1 : 0),
		    (f->arg2 ? *f->arg2 : 0));

#ifdef DEBUG
      printf("(C->S) %s (%c): %d\n", f->name, f->feature_type, f->value);
#endif
    }
}


void
        handleFeature(struct feature_cpacket *packet)
{
  checkFeature(packet);
}
#endif /* FEATURE_PACKETS */

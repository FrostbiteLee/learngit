#include <glib/gi18n.h>
#include <X11/Xatom.h>
#include <math.h>
#include <inttypes.h>
#include "savexrandr.h"
#include "edid.h"


typedef struct Vendor Vendor;
struct Vendor
{
    const char vendor_id[4];
    const char vendor_name[28];
};
/* This list of vendor codes derived from lshw
 *
 * http://ezix.org/project/wiki/HardwareLiSter
 *
 * Note: we now prefer to use data coming from hwdata (and shipped with
 * gnome-desktop). See
 * http://git.fedorahosted.org/git/?p=hwdata.git;a=blob_plain;f=pnp.ids;hb=HEAD
 * All contributions to the list of vendors should go there.
 */
static const struct Vendor vendors[] =
{
    { "AIC", "AG Neovo" },
    { "ACR", "Acer" },
    { "DEL", "DELL" },
    { "SAM", "SAMSUNG" },
    { "SNY", "SONY" },
    { "SEC", "Epson" },
    { "WAC", "Wacom" },
    { "NEC", "NEC" },
    { "CMO", "CMO" },	/* Chi Mei */
    { "BNQ", "BenQ" },

    { "ABP", "Advansys" },
    { "ACC", "Accton" },
    { "ACE", "Accton" },
    { "ADP", "Adaptec" },
    { "ADV", "AMD" },
    { "AIR", "AIR" },
    { "AMI", "AMI" },
    { "ASU", "ASUS" },
    { "ATI", "ATI" },
    { "ATK", "Allied Telesyn" },
    { "AZT", "Aztech" },
    { "BAN", "Banya" },
    { "BRI", "Boca Research" },
    { "BUS", "Buslogic" },
    { "CCI", "Cache Computers Inc." },
    { "CHA", "Chase" },
    { "CMD", "CMD Technology, Inc." },
    { "COG", "Cogent" },
    { "CPQ", "Compaq" },
    { "CRS", "Crescendo" },
    { "CSC", "Crystal" },
    { "CSI", "CSI" },
    { "CTL", "Creative Labs" },
    { "DBI", "Digi" },
    { "DEC", "Digital Equipment" },
    { "DBK", "Databook" },
    { "EGL", "Eagle Technology" },
    { "ELS", "ELSA" },
    { "ESS", "ESS" },
    { "FAR", "Farallon" },
    { "FDC", "Future Domain" },
    { "HWP", "Hewlett-Packard" },
    { "IBM", "IBM" },
    { "INT", "Intel" },
    { "ISA", "Iomega" },
    { "LEN", "Lenovo" },
    { "MDG", "Madge" },
    { "MDY", "Microdyne" },
    { "MET", "Metheus" },
    { "MIC", "Micronics" },
    { "MLX", "Mylex" },
    { "NVL", "Novell" },
    { "OLC", "Olicom" },
    { "PRO", "Proteon" },
    { "RII", "Racal" },
    { "RTL", "Realtek" },
    { "SCM", "SCM" },
    { "SKD", "SysKonnect" },
    { "SGI", "SGI" },
    { "SMC", "SMC" },
    { "SNI", "Siemens Nixdorf" },
    { "STL", "Stallion Technologies" },
    { "SUN", "Sun" },
    { "SUP", "SupraExpress" },
    { "SVE", "SVEC" },
    { "TCC", "Thomas-Conrad" },
    { "TCI", "Tulip" },
    { "TCM", "3Com" },
    { "TCO", "Thomas-Conrad" },
    { "TEC", "Tecmar" },
    { "TRU", "Truevision" },
    { "TOS", "Toshiba" },
    { "TYN", "Tyan" },
    { "UBI", "Ungermann-Bass" },
    { "USC", "UltraStor" },
    { "VDM", "Vadem" },
    { "VMI", "Vermont" },
    { "WDC", "Western Digital" },
    { "ZDS", "Zeos" },

    /* From http://faydoc.tripod.com/structures/01/0136.htm */
    { "ACT", "Targa" },
    { "ADI", "ADI" },
    { "AOC", "AOC Intl" },
    { "API", "Acer America" },
    { "APP", "Apple Computer" },
    { "ART", "ArtMedia" },
    { "AST", "AST Research" },
    { "CPL", "Compal" },
    { "CTX", "Chuntex Electronic Co." },
    { "DPC", "Delta Electronics" },
    { "DWE", "Daewoo" },
    { "ECS", "ELITEGROUP" },
    { "EIZ", "EIZO" },
    { "FCM", "Funai" },
    { "GSM", "LG Electronics" },
    { "GWY", "Gateway 2000" },
    { "HEI", "Hyundai" },
    { "HIT", "Hitachi" },
    { "HSL", "Hansol" },
    { "HTC", "Hitachi" },
    { "ICL", "Fujitsu ICL" },
    { "IVM", "Idek Iiyama" },
    { "KFC", "KFC Computek" },
    { "LKM", "ADLAS" },
    { "LNK", "LINK Tech" },
    { "LTN", "Lite-On" },
    { "MAG", "MAG InnoVision" },
    { "MAX", "Maxdata" },
    { "MEI", "Panasonic" },
    { "MEL", "Mitsubishi" },
    { "MIR", "miro" },
    { "MTC", "MITAC" },
    { "NAN", "NANAO" },
    { "NEC", "NEC Tech" },
    { "NOK", "Nokia" },
    { "OQI", "OPTIQUEST" },
    { "PBN", "Packard Bell" },
    { "PGS", "Princeton" },
    { "PHL", "Philips" },
    { "REL", "Relisys" },
    { "SDI", "Samtron" },
    { "SMI", "Smile" },
    { "SPT", "Sceptre" },
    { "SRC", "Shamrock Technology" },
    { "STP", "Sceptre" },
    { "TAT", "Tatung" },
    { "TRL", "Royal Information Company" },
    { "TSB", "Toshiba, Inc." },
    { "UNM", "Unisys" },
    { "VSC", "ViewSonic" },
    { "WTC", "Wen Tech" },
    { "ZCM", "Zenith Data Systems" },

    { "???", "Unknown" },
};

guint8 * xrandr_read_edid_data (Display  *xdisplay, RROutput  output);
static void xrandr_populate (xRandr *randr, Display   *xdisplay, GdkWindow *root_window);
static void xrandr_guess_relations (xRandr *randr);
static gchar *xrandr_friendly_name (xRandr *randr, guint output, int *succeed);
static Rotation xrandr_get_safe_rotations (xRandr *randr, Display *xdisplay, guint num_output);
static xRRMode *xrandr_list_supported_modes (XRRScreenResources *resources, XRROutputInfo *output_info);
static const char *find_vendor (const char *code);
//static void read_pnp_ids (void);
char * make_display_name (const MonitorInfo *info);
static void xrandr_cleanup (xRandr *randr);
void xrandr_free (xRandr *randr);

static GHashTable *pnp_ids = NULL;

xRandr *
xrandr_new (GdkDisplay  *display,
                GError     **error)
{
    xRandr *randr;
    Display   *xdisplay;
    GdkWindow *root_window;
    gint       major, minor;

    g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);
    g_return_val_if_fail (error == NULL || *error == NULL, NULL);

    /* get the x display */
    xdisplay = gdk_x11_display_get_xdisplay (display);

    /* check if the randr extension is available */
    if (XRRQueryVersion (xdisplay, &major, &minor) == FALSE)
    {
        g_set_error (error, 0, 0, _("Unable to query the version of the RandR extension being used"));
        return NULL;
    }

    /* we need atleast randr 1.2, 2.0 will probably break the api */
    if (major < 1 || (major == 1 && minor < 2))
    {
        /* 1.2 is required */
        g_set_error (error, 0, 0, _("This system is using RandR %d.%d. For the display settings to work "
                                    "version 1.2 is required at least"), major, minor);
        return NULL;
    }

    /* allocate the structure */
    randr = g_slice_new0 (xRandr);
    randr->priv = g_slice_new0 (xRandrPrivate);

    randr->priv->has_1_3 = (major > 1 || (major == 1 && minor >= 3));

    /* set display */
    randr->priv->display = display;

    /* get the root window */
    root_window = gdk_get_default_root_window ();

    /* get the screen resource */
    randr->priv->resources = XRRGetScreenResources (xdisplay, GDK_WINDOW_XID (root_window));

    xrandr_populate (randr, xdisplay, root_window);

    return randr;
}

static xRRMode *
xrandr_list_supported_modes (XRRScreenResources *resources,
                                XRROutputInfo       *output_info)
{
    xRRMode *modes;
    gint m, n;

    g_return_val_if_fail (resources != NULL, NULL);
    g_return_val_if_fail (output_info != NULL, NULL);

    if (output_info->nmode == 0)
        return NULL;

    modes = g_new0 (xRRMode, output_info->nmode);

    for (n = 0; n < output_info->nmode; ++n)
    {
        modes[n].id = output_info->modes[n];

        /* we need to walk yet another list to get the mode info */
        for (m = 0; m < resources->nmode; ++m)
        {
            if (output_info->modes[n] == resources->modes[m].id)
            {
                modes[n].width = resources->modes[m].width;
                modes[n].height = resources->modes[m].height;
                modes[n].rate = (gdouble) resources->modes[m].dotClock /
                                ((gdouble) resources->modes[m].hTotal * (gdouble) resources->modes[m].vTotal);

                break;
            }
        }
    }

    return modes;
}

static Rotation
xrandr_get_safe_rotations (xRandr *randr,
                               Display   *xdisplay,
                               guint      num_output)
{
    XRRCrtcInfo *crtc_info;
    Rotation     rot;
    gint         n;

    g_return_val_if_fail (num_output < randr->noutput, RR_Rotate_0);
    g_return_val_if_fail (randr->priv->output_info[num_output]->ncrtc > 0, RR_Rotate_0);

    rot = X_RANDR_ROTATIONS_MASK | X_RANDR_REFLECTIONS_MASK;
    for (n = 0; n < randr->priv->output_info[num_output]->ncrtc; ++n)
    {
        crtc_info = XRRGetCrtcInfo (xdisplay, randr->priv->resources,
                                    randr->priv->output_info[num_output]->crtcs[n]);
        rot &= crtc_info->rotations;
        XRRFreeCrtcInfo (crtc_info);
    }

    return rot;
}
#if 0
static void
read_pnp_ids (void)
{
    gchar *contents;
    gchar **lines;
    gchar *line;
    gchar *code, *name;
    gint i;

    if (pnp_ids)
        return;

    pnp_ids = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

    if (g_file_get_contents (PNP_IDS, &contents, NULL, NULL))
    {
        lines = g_strsplit (contents, "\n", -1);
        for (i = 0; lines[i]; i++)
        {
             line = lines[i];
             if (line[3] == '\t')
             {
                 code = line;
                 line[3] = '\0';
                 name = line + 4;
                 g_hash_table_insert (pnp_ids, code, name);
             }
        }
        g_free (lines);
        g_free (contents);
    }
}
#endif

static const char *
find_vendor (const char *code)
{
    const char *vendor_name;
    unsigned int i;

    //read_pnp_ids ();
    pnp_ids = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

    vendor_name = g_hash_table_lookup (pnp_ids, code);

    if (vendor_name)
        return vendor_name;

    for (i = 0; i < sizeof (vendors) / sizeof (vendors[0]); ++i)
    {
	const Vendor *v = &(vendors[i]);

	if (strcmp (v->vendor_id, code) == 0)
	    return v->vendor_name;
    }

    return code;
};

xRandrRecordMonitor *
xrandrrecordmonitor_new()
{
	xRandrRecordMonitor *monitor;

	monitor = g_slice_new0 (xRandrRecordMonitor);

	memset(monitor->manufacturer_code, 0, sizeof(monitor->manufacturer_code));
	monitor->product_code = -1;
	memset(monitor->port, 0, sizeof(monitor->port));
	monitor->enable = TRUE;
	memset(monitor->mode, 0, sizeof(monitor->mode));
	monitor->primary = FALSE;
	memset(monitor->rotation, 0, sizeof(monitor->rotation));

	return monitor;
}

xRandrRecord *
xrandrrecord_new()
{
	xRandrRecord *record;

	record = g_slice_new0 (xRandrRecord);

	record->count = 0;
	record->monitor1 = xrandrrecordmonitor_new();
	record->monitor2 = xrandrrecordmonitor_new();
	record->monitor3 = xrandrrecordmonitor_new();
	record->pNext = NULL;

	return record;
}

void 
xrandrrecordmonitor_copy(xRandrRecordMonitor *dest, xRandrRecordMonitor *src)
{
	if( !dest || !src)
	{
		return;
	}
	
	strncpy(dest->manufacturer_code, src->manufacturer_code, sizeof(dest->manufacturer_code));	
	dest->product_code = src->product_code;
	strncpy(dest->port, src->port, sizeof(dest->port));
	dest->enable = src->enable;
	strncpy(dest->mode, src->mode, sizeof(dest->mode));
	dest->primary = src->primary;
	strncpy(dest->rotation, src->rotation, sizeof(dest->rotation));
}

xRandrRecord * 
xrandrrecord_prepend(xRandrRecord *list, xRandrRecord *node)
{
	if(list == NULL || node == NULL)
	{
		return;
	}

	node->pNext = list;

	return node;
}

void
xrandrrecord_append(xRandrRecord *list, xRandrRecord *node)
{
	if(list == NULL || node == NULL)
	{
		return;
	}

	if(list->count == 0)
	{
		list->count = node->count;
		if(node->monitor1 != NULL)
		{
			list->monitor1 = xrandrrecordmonitor_new();
			xrandrrecordmonitor_copy(list->monitor1, node->monitor1);
		}
		if(node->monitor2 != NULL)
		{
			list->monitor2 = xrandrrecordmonitor_new();
			xrandrrecordmonitor_copy(list->monitor2, node->monitor2);
		}
		if(node->monitor3 != NULL)
		{
			list->monitor3 = xrandrrecordmonitor_new();
			xrandrrecordmonitor_copy(list->monitor3, node->monitor3);
		}
		xrandrrecord_free(node);

		return;
	}
	
	xRandrRecord *tmp = list;
	while(tmp->pNext != NULL)
	{
		tmp = tmp->pNext;
	}
	tmp->pNext = node;
	
	return;
}

void
xrandrrecord_free(xRandrRecord *record)
{
	xRandrRecord *r1;

	while(record)
	{
		r1 = record;
		record = record->pNext;
		r1->count = 0;
		if(r1->monitor1)
		{
			g_slice_free (xRandrRecordMonitor, r1->monitor1);
			r1->monitor1 = NULL;
		}
		if(r1->monitor2)
		{
			g_slice_free (xRandrRecordMonitor, r1->monitor2);
			r1->monitor2 = NULL;
		}
		if(r1->monitor3)
		{
			g_slice_free (xRandrRecordMonitor, r1->monitor3);
			r1->monitor3 = NULL;
		}
		r1->pNext = NULL;
		g_slice_free (xRandrRecord, r1);
		r1 = NULL;
	}
}

xRandrRecord *
xrandrrecord_del(xRandrRecord *record, int index)
{
	xRandrRecord *r1, *r2;

	if(index == 1)
	{
		r1 = record;
		record = record->pNext;
		r1->pNext = NULL;
		xrandrrecord_free(r1);

		return record;
	}

	r1 = record;
	r2 = record->pNext;
	while(index-2 && r2)
	{
		r1 = r2;
		r2 = r2->pNext;
		index--;
	}
	if(r2)
	{
		r1->pNext = r2->pNext;
		r2->pNext = NULL;
		xrandrrecord_free(r2);
	}
	return record;
}

int
xrandr_get_vendor(xRandr *randr,
                          	guint output,
				xRandrRecordMonitor *monitor)
{
	Display     *xdisplay;	
	MonitorInfo *info = NULL;
	int	success = 0;
	guint8      *edid_data;
	
	xdisplay = gdk_x11_display_get_xdisplay (randr->priv->display);
    	edid_data = xrandr_read_edid_data (xdisplay, randr->priv->resources->outputs[output]);

	if (edid_data)
    	{
        	info = decode_edid (edid_data);
    	}
	else
	{
		return success;
	}
	if (info)
	{
		strncpy(monitor->manufacturer_code, info->manufacturer_code, sizeof(monitor->manufacturer_code));
		monitor->product_code = info->product_code;
		strncpy(monitor->port, randr->priv->output_info[output]->name, sizeof(monitor->port));
		success = 1;
	}
	if (info)
	{
		g_free (info);
	}
	if (edid_data)
	{
    		g_free (edid_data);
	}

	return success;
}
int
parse_xrandr_command(char *cmd, xRandrRecord *record)
{
	char *str, *strS;
	xRandrRecord *tmprecord = record;
	xRandrRecordMonitor *monitor = NULL;
	int disablenum = 0, icopy = 0;
	
	str = strtok_r(cmd, " ", &strS);
	while(str)
	{
		if(strcmp(str, "--output") == 0)
		{
			str = strtok_r(NULL, " ", &strS);
			if(str)
			{
				if(  tmprecord->monitor1 
				&& strcmp(str, tmprecord->monitor1->port) == 0)
				{
					monitor = tmprecord->monitor1;
					tmprecord->count++;
				}
				else if(  tmprecord->monitor2
				&& strcmp(str, tmprecord->monitor2->port) == 0)
				{
					monitor = tmprecord->monitor2;
					tmprecord->count++;
				}
				else if(  tmprecord->monitor3
				&& strcmp(str, tmprecord->monitor3->port) == 0)
				{
					monitor = tmprecord->monitor3;
					tmprecord->count++;
				}
			}
		}
		else if(monitor && strcmp(str, "--mode") == 0)
		{
			str = strtok_r(NULL, " ", &strS);
			if(str)
			{
				strncpy(monitor->mode, str, sizeof(monitor->mode));
			}
		}
		else if(monitor && strcmp(str, "--primary") == 0)
		{
			monitor->primary = TRUE;
		}
		else if(monitor && strcmp(str, "--rotation") == 0)
		{
			str = strtok_r(NULL, " ", &strS);
			if(str)
			{
				strncpy(monitor->rotation, str, sizeof(monitor->rotation));
			}
		}
		else if(monitor && strcmp(str, "--off") == 0)
		{
			//tmprecord->count--;
			monitor->enable = FALSE;
			disablenum++;
		}
		else if(monitor && strcmp(str, "--same-as") == 0)
		{
			icopy = 1;
		}
		str = strtok_r(NULL, " ", &strS);
	}
	if( (tmprecord->count - disablenum) == 1
	|| icopy == 1)
	{
		tmprecord->monitor1->primary = FALSE;
		tmprecord->monitor2->primary = FALSE;
		tmprecord->monitor3->primary = FALSE;
	}
	if(tmprecord->count == 0)
		return 0;
	else
		return 1;
}
#if 0
static int 
xrandr_get_record_count(xRandrRecord *record)
{
	int count = 0;
	xRandrRecord *tmprecord = record;
	
	while(tmprecord)
	{
		if(tmprecord->enable == 1)
		{
			count++;
		}
		tmprecord = tmprecord->pNext;
	}
	return count;
}
#endif
void
xrandr_set_keyfile(GKeyFile* kf, 
				char *grp, 
				int num, 
				xRandrRecordMonitor *monitor)
{
	if(!kf || !grp || !monitor)
		return;

	char keyname[20];

	memset(keyname, 0, sizeof(keyname));
	sprintf(keyname, "ManufacturerCode%d", num);
	g_key_file_set_string( kf, grp, keyname, monitor->manufacturer_code);
	memset(keyname, 0, sizeof(keyname));
	sprintf(keyname, "ProductCode%d", num);
	g_key_file_set_integer( kf, grp, keyname, monitor->product_code);
	memset(keyname, 0, sizeof(keyname));
	sprintf(keyname, "Port%d", num);
	g_key_file_set_string( kf, grp, keyname, monitor->port);
	memset(keyname, 0, sizeof(keyname));
	sprintf(keyname, "Enable%d", num);
	g_key_file_set_boolean( kf, grp, keyname, monitor->enable);
	memset(keyname, 0, sizeof(keyname));
	sprintf(keyname, "Mode%d", num);
	g_key_file_set_string( kf, grp, keyname, monitor->mode);
	memset(keyname, 0, sizeof(keyname));
	sprintf(keyname, "Primary%d", num);
	g_key_file_set_boolean( kf, grp, keyname, monitor->primary);
	memset(keyname, 0, sizeof(keyname));
	sprintf(keyname, "Rotation%d", num);
	g_key_file_set_string( kf, grp, keyname, monitor->rotation);
}

void 
xrandr_get_record_monitor_info(GKeyFile* kf, 
						char *grp, 
						int num, 
						xRandrRecordMonitor *monitor)
{
	char keyname[20];
	gchar *str;
	 
	memset(keyname, 0, sizeof(keyname));
	sprintf(keyname, "ManufacturerCode%d", num);
	str = g_key_file_get_string(kf, grp, keyname, NULL);
	if(str != NULL)
	{
		strncpy(monitor->manufacturer_code, str, sizeof(monitor->manufacturer_code));
	}
	else
	{
		return;
	}

	memset(keyname, 0, sizeof(keyname));
	sprintf(keyname, "ProductCode%d", num);
	monitor->product_code = g_key_file_get_integer(kf, grp, keyname, NULL);

	memset(keyname, 0, sizeof(keyname));
	sprintf(keyname, "Port%d", num);
	str = g_key_file_get_string(kf, grp, keyname, NULL);
	if(str != NULL)
	{
		strncpy(monitor->port, str, sizeof(monitor->port));
	}
	else
	{
		return;
	}

	memset(keyname, 0, sizeof(keyname));
	sprintf(keyname, "Enable%d", num);
	monitor->enable = g_key_file_get_boolean(kf, grp, keyname, NULL);

	memset(keyname, 0, sizeof(keyname));
	sprintf(keyname, "Mode%d", num);
	str = g_key_file_get_string(kf, grp, keyname, NULL);
	if(str != NULL)
	{
		strncpy(monitor->mode, str, sizeof(monitor->mode));
	}
	else
	{
		return;
	}

	memset(keyname, 0, sizeof(keyname));
	sprintf(keyname, "Primary%d", num);
	monitor->primary = g_key_file_get_boolean(kf, grp, keyname, NULL);

	memset(keyname, 0, sizeof(keyname));
	sprintf(keyname, "Rotation%d", num);
	str = g_key_file_get_string(kf, grp, keyname, NULL);
	if(str != NULL)
	{
		strncpy(monitor->rotation, str, sizeof(monitor->rotation));
	}
	else
	{
		return;
	}
}

int
xrandr_read_record(GKeyFile* kf, 
				char *grp,
				xRandrRecord *record)
{
	if(!kf || !grp || !record)
		return 0;

	int count;

	count = g_key_file_get_integer(kf, grp, "DisplayNum", NULL);
	if(count != 1 && count != 2 && count != 3)
	{
		return 0;
	}
	record->count = count;
	if(strcmp(record->monitor1->manufacturer_code, "") == 0)
	{
		xrandr_get_record_monitor_info(kf, grp, 1, record->monitor1);
	}
	if(strcmp(record->monitor2->manufacturer_code, "") == 0)
	{
		xrandr_get_record_monitor_info(kf, grp, 2, record->monitor2);
	}
	if(strcmp(record->monitor3->manufacturer_code, "") == 0)
	{
		xrandr_get_record_monitor_info(kf, grp, 3, record->monitor3);
	}

	return 1;
}

void
xrandr_read_record_list(GKeyFile* kf, 
				xRandrRecord *record)
{
	if(!kf || !record)
		return;

	char grp[10];
	int irecord = 1;
	xRandrRecord *recordnode;

	while(1)
	{
		memset(grp, 0, sizeof(grp));
		snprintf(grp, sizeof(grp), "RECORD%d", irecord);
		recordnode = xrandrrecord_new();
		if(1 == xrandr_read_record(kf, grp, recordnode))
		{
			xrandrrecord_append(record, recordnode);
		}
		else
		{
			break;
		}
		irecord++;
	}
}

static int
match_current_record_monitor(xRandrRecordMonitor *monitor1, 
						xRandrRecordMonitor *monitor2) 
{
	int match = 0;

	if(  strcmp(monitor1->manufacturer_code, monitor2->manufacturer_code) == 0
	&& monitor1->product_code == monitor2->product_code
	&& strcmp(monitor1->port, monitor2->port) == 0)
	{
		match = 1;
	}

	return match;
}

void print_record(xRandrRecord *recordlist)
{
	xRandrRecord *m = recordlist;
	
	while(m)
	{
		if(strcmp(m->monitor1->manufacturer_code, "") != 0)
		{
			printf("fac1:%s\n", m->monitor1->manufacturer_code);
			printf("product1:%d\n", m->monitor1->product_code);
			printf("port1:%s\n", m->monitor1->port);
			printf("enable1:%d\n", m->monitor1->enable);
			printf("mode1:%s\n", m->monitor1->mode);
			printf("primary1:%d\n", m->monitor1->primary);
			printf("rotation1:%s\n", m->monitor1->rotation);
		}
		if(strcmp(m->monitor2->manufacturer_code, "") != 0)
		{
			printf("fac2:%s\n", m->monitor2->manufacturer_code);
			printf("product2:%d\n", m->monitor2->product_code);
			printf("port2:%s\n", m->monitor2->port);
			printf("enable2:%d\n", m->monitor2->enable);
			printf("mode2:%s\n", m->monitor2->mode);
			printf("primary2:%d\n", m->monitor2->primary);
			printf("rotation2:%s\n", m->monitor2->rotation);
		}
		if(strcmp(m->monitor3->manufacturer_code, "") != 0)
		{
			printf("fac3:%s\n", m->monitor3->manufacturer_code);
			printf("product3:%d\n", m->monitor3->product_code);
			printf("port3:%s\n", m->monitor3->port);
			printf("enable3:%d\n", m->monitor3->enable);
			printf("mode3:%s\n", m->monitor3->mode);
			printf("primary3:%d\n", m->monitor3->primary);
			printf("rotation3:%s\n", m->monitor3->rotation);
		}
		
		m = m->pNext;
	}
}

void
xrandr_write_record2(xRandrRecord *recordlist, GKeyFile* kf)
{
	xRandrRecord *recordnode = recordlist;
	int irecord = 1;
	char *file = NULL, *data = NULL;
    	gsize len;
	char grp[10];

	while(recordnode)
	{
		if(recordnode->count != 0)
		{
			snprintf(grp, sizeof(grp), "RECORD%d", irecord);
			g_key_file_set_integer (kf, grp, "DisplayNum", recordnode->count);
		}
		if(strcmp(recordnode->monitor1->manufacturer_code, "") != 0)
		{
			xrandr_set_keyfile(kf, grp, 1, recordnode->monitor1);
		}
		if(strcmp(recordnode->monitor2->manufacturer_code, "") != 0)
		{
			xrandr_set_keyfile(kf, grp, 2, recordnode->monitor2);
		}
		if(strcmp(recordnode->monitor3->manufacturer_code, "") != 0)
		{
			xrandr_set_keyfile(kf, grp, 3, recordnode->monitor3);
		}
		recordnode = recordnode->pNext;
		irecord++;
	}
	data = g_key_file_to_data(kf, &len, NULL);
	file = g_build_filename(  g_get_user_config_dir(),
		                      "xrandrrecord.ini",
		                      NULL );
	g_file_set_contents(file, data, len, NULL);

    	if(file != NULL) 
	{
		g_free(file);	
		file = NULL;
	}
    	if(data != NULL) 
	{
		g_free(data);
		data = NULL;
	}
}

void
update_record2(xRandrRecord *record)
{
	xRandrRecord *oldrecordlist;
	xRandrRecord *recordnode;
	GKeyFile* kf = NULL;
	char filename[100];
	int recordnum = X_RANDR_RECORD_COUNT;

	oldrecordlist = xrandrrecord_new();
	kf = g_key_file_new();
	memset(filename, 0, sizeof(filename));
	snprintf(filename, sizeof(filename), "%s/xrandrrecord.ini", g_get_user_config_dir());
	if(TRUE == g_key_file_load_from_file(kf, filename, G_KEY_FILE_NONE, NULL))
	{
		xrandr_read_record_list(kf, oldrecordlist);
		g_key_file_free (kf);	
		kf = NULL;
	}

	recordnode = oldrecordlist;
	while(recordnode && recordnum)
	{
		if(recordnode->count == record->count)
		{
			if(match_current_record_monitor(recordnode->monitor1, record->monitor1) == 1)
			{
				if(  match_current_record_monitor(recordnode->monitor2, record->monitor2) == 1
				&& match_current_record_monitor(recordnode->monitor3, record->monitor3) == 1)
				{
					break;
				}
				else if(  match_current_record_monitor(recordnode->monitor2, record->monitor3) == 1
					&& match_current_record_monitor(recordnode->monitor3, record->monitor2) == 1)
				{
					break;
				}
			}
			else if(match_current_record_monitor(recordnode->monitor1, record->monitor2) == 1)
			{
				if(  match_current_record_monitor(recordnode->monitor2, record->monitor1) == 1
				&& match_current_record_monitor(recordnode->monitor3, record->monitor3) == 1)
				{
					break;
				}
				else if(  match_current_record_monitor(recordnode->monitor2, record->monitor3) == 1
					&& match_current_record_monitor(recordnode->monitor3, record->monitor1) == 1)
				{
					break;
				}
			}
			else if(match_current_record_monitor(recordnode->monitor1, record->monitor3) == 1)
			{
				if(  match_current_record_monitor(recordnode->monitor2, record->monitor1) == 1
				&& match_current_record_monitor(recordnode->monitor3, record->monitor2) == 1)
				{
					break;
				}
				else if(  match_current_record_monitor(recordnode->monitor2, record->monitor2) == 1
					&& match_current_record_monitor(recordnode->monitor3, record->monitor1) == 1)
				{
					break;
				}
			}
		}
		recordnode = recordnode->pNext;
		recordnum--;
	}//while
	
	if(recordnum > 0 && recordnode)
	{
		oldrecordlist = xrandrrecord_del(oldrecordlist, X_RANDR_RECORD_COUNT-recordnum+1);
	}
	else if(recordnum == 0)
	{
		oldrecordlist = xrandrrecord_del(oldrecordlist, X_RANDR_RECORD_COUNT);
	}
	record->pNext = oldrecordlist;
	if(kf == NULL)
	{
		kf = g_key_file_new();
	}
	xrandr_write_record2(record, kf);

	xrandrrecord_free(oldrecordlist);
	if(kf != NULL) 
	{
		g_key_file_free (kf);
		kf = NULL;
	}
}

void
update_record(xRandrRecord *record)
{
	xRandrRecord *recordlist;
	xRandrRecord *recordnode;
	xRandrRecord *tmprecord;
    	GKeyFile* kf = NULL;
	char filename[100];
	int recordnum = X_RANDR_RECORD_COUNT;

	recordlist = xrandrrecord_new();
	tmprecord = xrandrrecord_new();
	tmprecord->count = record->count;
	xrandrrecordmonitor_copy(tmprecord->monitor1, record->monitor1);
	xrandrrecordmonitor_copy(tmprecord->monitor2, record->monitor2);
	xrandrrecordmonitor_copy(tmprecord->monitor3, record->monitor3);
	tmprecord->pNext = record->pNext;
	kf = g_key_file_new();
	memset(filename, 0, sizeof(filename));
	snprintf(filename, sizeof(filename), "%s/xrandrrecord.ini", g_get_user_config_dir());
	if(TRUE == g_key_file_load_from_file(kf, filename, G_KEY_FILE_NONE, NULL))
	{
		xrandr_read_record_list(kf, recordlist);
		g_key_file_free (kf);	
		kf = NULL;
	}
	recordnode = recordlist;
	while(recordnode && recordnum)
	{
		if(  recordnode->count != 0
		&& recordnode->count == tmprecord->count)
		{
			if(tmprecord->count == 1)
			{
				if(  tmprecord->monitor1->enable == 1 
				&& match_current_record_monitor(recordnode->monitor1, tmprecord->monitor1) == 1)
				{
					xrandrrecordmonitor_copy(recordnode->monitor1, tmprecord->monitor1);
					break;
				}
				else if(  tmprecord->monitor2->enable == 1 
				&& match_current_record_monitor(recordnode->monitor1, tmprecord->monitor2) == 1)
				{
					xrandrrecordmonitor_copy(recordnode->monitor1, tmprecord->monitor2);
					break;
				}
			}
			else if(  recordnode->count == 2
				&& recordnode->monitor1->enable == 1
				&& recordnode->monitor2->enable == 1)
			{
				if( (match_current_record_monitor(recordnode->monitor1, tmprecord->monitor1) == 1
				&& match_current_record_monitor(recordnode->monitor2, tmprecord->monitor2) == 1)
				|| (match_current_record_monitor(recordnode->monitor1, tmprecord->monitor2) == 1
				&& match_current_record_monitor(recordnode->monitor2, tmprecord->monitor1) == 1))
				{
					xrandrrecordmonitor_copy(recordnode->monitor1, tmprecord->monitor1);
					xrandrrecordmonitor_copy(recordnode->monitor2, tmprecord->monitor2);
					break;
				}
			}
		}
		recordnode = recordnode->pNext;
		recordnum--;
	}//WHILE 
	if(recordlist)
	{
		xrandrrecord_append(recordlist, tmprecord);
		if(  recordnum == 0 
		&& !recordnode)
		{
			recordlist = xrandrrecord_del(recordlist, 1);
		}
		else if(  recordnum > 0
			&& recordnode)
		{
			recordlist = xrandrrecord_del(recordlist, X_RANDR_RECORD_COUNT - recordnum +1);
		}
		/*if(!recordnode)
		{
			xrandrrecord_add(recordlist, tmprecord);
		}
		//print_record(recordlist);
		if(recordnum == 0)
		{
			recordlist = xrandrrecord_del(recordlist, 1);
		}*/
		//print_record(recordlist);
		if(kf == NULL)
		{
			kf = g_key_file_new();
		}
		xrandr_write_record2(recordlist, kf);
		xrandrrecord_free(recordlist);
	}

	if(kf != NULL) 
	{
		g_key_file_free (kf);
		kf = NULL;
	}
}

static gchar *
xrandr_friendly_name (xRandr *randr,
                          	guint output,
				int *succeed)
{
    Display     *xdisplay;
    MonitorInfo *info = NULL;
    guint8      *edid_data;
    gchar       *friendly_name = NULL;
    const gchar *name = randr->priv->output_info[output]->name;

    /* special case, a laptop */
    if (g_str_has_prefix (name, "LVDS")
        || strcmp (name, "PANEL") == 0)
        return g_strdup (_("Laptop"));

    /* otherwise, get the vendor & size */
    xdisplay = gdk_x11_display_get_xdisplay (randr->priv->display);
    edid_data = xrandr_read_edid_data (xdisplay, randr->priv->resources->outputs[output]);
    
    if (edid_data)
    {
        info = decode_edid (edid_data);
    }
    if (info)
    {
        friendly_name = make_display_name (info);
    }
    g_free (info);
    g_free (edid_data);

    if (friendly_name)
    {
	*succeed = 1;
        return friendly_name;
    }
	//printf("the other name\n");
    /* last attempt to return a better name */
    if (g_str_has_prefix (name, "VGA")
             || g_str_has_prefix (name, "Analog"))
        return g_strdup (_("Monitor"));
    else if (g_str_has_prefix (name, "TV")
             || strcmp (name, "S-video") == 0)
        return g_strdup (_("Television"));
    else if (g_str_has_prefix (name, "TMDS")
             || g_str_has_prefix (name, "DVI")
             || g_str_has_prefix (name, "Digital"))
        return g_strdup (_("Digital display"));

    /* everything failed, fallback */
    return g_strdup (name);
}

char *
make_display_name (const MonitorInfo *info)
{
    const char *vendor;
    int width_mm, height_mm, inches;

    if (info)
    {
	vendor = find_vendor (info->manufacturer_code);
    }
    else
    {
        /* Translators: "Unknown" here is used to identify a monitor for which
         * we don't know the vendor. When a vendor is known, the name of the
         * vendor is used. */
	vendor = C_("Monitor vendor", "Unknown");
    }

    if (info && info->width_mm != -1 && info->height_mm)
    {
	width_mm = info->width_mm;
	height_mm = info->height_mm;
    }
    else if (info && info->n_detailed_timings)
    {
	width_mm = info->detailed_timings[0].width_mm;
	height_mm = info->detailed_timings[0].height_mm;
    }
    else
    {
	width_mm = -1;
	height_mm = -1;
    }

    if (width_mm != -1 && height_mm != -1)
    {
	//printf("width_mm:%d__height_mm:%d\n",width_mm, height_mm);
	double d = sqrt (width_mm * width_mm + height_mm * height_mm);
	//printf("d:%0.3f", d);
	inches = (int)(d / 25.4 + 0.5);
    }
    else
    {
	inches = -1;
    }

    if (inches > 0)
	return g_strdup_printf ("%s %d\"", vendor, inches);
    else
	return g_strdup (vendor);
}

static void
xrandr_guess_relations (xRandr *randr)
{
    guint n, m;

    /* walk the connected outputs */
    for (n = 0; n < randr->noutput; ++n)
    {
        /* ignore relations for inactive outputs */
        if (randr->mode[n] == None)
            continue;

        for (m = 0; m < randr->noutput; ++m)
        {
            /* additionally ignore itself */
            if (randr->mode[m] == None || m == n)
                continue;

            /* horizontal scale */
            if (randr->priv->position[n].x == randr->priv->position[m].x)
            {
                if (randr->priv->position[n].y == randr->priv->position[m].y)
                    randr->relation[n] = X_RANDR_PLACEMENT_MIRROR;
                else if (randr->priv->position[n].y > randr->priv->position[m].y)
                    randr->relation[n] = X_RANDR_PLACEMENT_DOWN;
                else
                    randr->relation[n] = X_RANDR_PLACEMENT_UP;

                randr->related_to[n] = m;
                break;
            }

            /* vertical scale */
            if (randr->priv->position[n].y == randr->priv->position[m].y)
            {
                if (randr->priv->position[n].x == randr->priv->position[m].x)
                    randr->relation[n] = X_RANDR_PLACEMENT_MIRROR;
                else if (randr->priv->position[n].x > randr->priv->position[m].x)
                    randr->relation[n] = X_RANDR_PLACEMENT_RIGHT;
                else
                    randr->relation[n] = X_RANDR_PLACEMENT_LEFT;

                randr->related_to[n] = m;
                break;
            }
        }
    }
}


static void
xrandr_populate (xRandr *randr,
                     Display   *xdisplay,
                     GdkWindow *root_window)
{
    GPtrArray     *outputs;
    XRROutputInfo *output_info;
    XRRCrtcInfo   *crtc_info;
    gint           n;
    guint          m;
    int 	succeed;

    g_return_if_fail (randr != NULL);
    g_return_if_fail (randr->priv != NULL);
    g_return_if_fail (randr->priv->resources != NULL);

    /* prepare the temporary cache */
    outputs = g_ptr_array_new ();

    /* walk the outputs */
    for (n = 0; n < randr->priv->resources->noutput; ++n)
    {
        /* get the output info */
        output_info = XRRGetOutputInfo (xdisplay, randr->priv->resources,
                                        randr->priv->resources->outputs[n]);

        /* forget about disconnected outputs 
        if (output_info->connection != RR_Connected)
        {
            XRRFreeOutputInfo (output_info);
            continue;
        }*/

        /* cache it */
        g_ptr_array_add (outputs, output_info);
    }

    /* migrate the temporary cache */
    randr->noutput = outputs->len;
    randr->priv->output_info = (XRROutputInfo **) g_ptr_array_free (outputs, FALSE);

    /* allocate final space for the settings */
    randr->mode = g_new0 (RRMode, randr->noutput);
    randr->priv->modes = g_new0 (xRRMode *, randr->noutput);
    randr->priv->position = g_new0 (xOutputPosition, randr->noutput);
    randr->rotation = g_new0 (Rotation, randr->noutput);
    randr->rotations = g_new0 (Rotation, randr->noutput);
    randr->relation = g_new0 (xOutputRelation, randr->noutput);
    randr->related_to = g_new0 (guint, randr->noutput);
    randr->status = g_new0 (xOutputStatus, randr->noutput);
    randr->friendly_name = g_new0 (gchar *, randr->noutput);

    /* walk the connected outputs */
    for (m = 0; m < randr->noutput; ++m)
    {//printf("there m is what?%d\n", m);
        /* fill in supported modes */
        randr->priv->modes[m] = xrandr_list_supported_modes (randr->priv->resources, randr->priv->output_info[m]);

#ifdef HAS_RANDR_ONE_POINT_THREE
        /* find the primary screen if supported */
        if (randr->priv->has_1_3 && XRRGetOutputPrimary (xdisplay, GDK_WINDOW_XID (root_window)) == randr->priv->resources->outputs[m])
            randr->status[m] = X_OUTPUT_STATUS_PRIMARY;
        else
#endif
            randr->status[m] = X_OUTPUT_STATUS_SECONDARY;

        if (randr->priv->output_info[m]->crtc != None)
        {
            crtc_info = XRRGetCrtcInfo (xdisplay, randr->priv->resources,
                                        randr->priv->output_info[m]->crtc);
            randr->mode[m] = crtc_info->mode;
            randr->rotation[m] = crtc_info->rotation;
            randr->rotations[m] = crtc_info->rotations;
            randr->priv->position[m].x = crtc_info->x;
            randr->priv->position[m].y = crtc_info->y;
            XRRFreeCrtcInfo (crtc_info);
        }
        else
        {
            /* output disabled */
            randr->mode[m] = None;
            randr->rotation[m] = RR_Rotate_0;
            randr->rotations[m] = xrandr_get_safe_rotations (randr, xdisplay, m);
        }

        /* fill in the name used by the UI */
	//printf("xid: 0x%08x", randr->priv->resources->outputs[m]);
        randr->friendly_name[m] = xrandr_friendly_name (randr, m, &succeed);
	if(succeed == 1)
	{
		printf("the friendlyname is %s.\n", randr->friendly_name[m]);
		succeed = 0;
	}
    }

    /* calculate relations from positions */
    xrandr_guess_relations (randr);
}

static void
print_edid(int nitems, const unsigned char *prop)
{
    int k;
    printf ("\n\t\t");
    for (k = 0; k < nitems; k++)
    {
    if (k != 0 && (k % 16) == 0)
    {
        printf ("\n\t\t");
    }
    printf("%02" PRIx8, prop[k]);
    }
    printf("\n");
}

guint8 *
xrandr_read_edid_data (Display  *xdisplay,
                           RROutput  output)
{
    unsigned char *prop;
    int            actual_format;
    unsigned long  nitems, bytes_after;
    Atom           actual_type;
    Atom           edid_atom;
    guint8        *result = NULL;

    edid_atom = gdk_x11_get_xatom_by_name (RR_PROPERTY_RANDR_EDID);

    if (edid_atom != None)
    {
        if (XRRGetOutputProperty (xdisplay, output, edid_atom, 0, 100,
                                  False, False, AnyPropertyType,
                                  &actual_type, &actual_format, &nitems,
                                  &bytes_after, &prop) == Success)
        {
            if (actual_type == XA_INTEGER && actual_format == 8)
	    {
		//print_edid(nitems, prop);
                result = g_memdup (prop, nitems);
            }
        }

        XFree (prop);
    }

    return result;
}

static void
xrandr_cleanup (xRandr *randr)
{
    guint n;

    /* free the output/mode info cache */
    for (n = 0; n < randr->noutput; ++n)
    {
        if (G_LIKELY (randr->priv->output_info[n]))
            XRRFreeOutputInfo (randr->priv->output_info[n]);
        if (G_LIKELY (randr->priv->modes[n]))
            g_free (randr->priv->modes[n]);
        if (G_LIKELY (randr->friendly_name[n]))
            g_free (randr->friendly_name[n]);
    }

    /* free the screen resources */
    XRRFreeScreenResources (randr->priv->resources);

    /* free the settings */
    g_free (randr->friendly_name);
    g_free (randr->mode);
    g_free (randr->priv->modes);
    g_free (randr->rotation);
    g_free (randr->rotations);
    g_free (randr->status);
    g_free (randr->relation);
    g_free (randr->related_to);
    g_free (randr->priv->position);
    g_free (randr->priv->output_info);
}

void
xrandr_free (xRandr *randr)
{
    xrandr_cleanup (randr);

    /* free the structure */
    g_slice_free (xRandrPrivate, randr->priv);
    g_slice_free (xRandr, randr);
}

#define XRANDERCONFIG	"/tmp/.xrandr.0"

gboolean 
Write_Monitorhotplug_Config(xRandrRecord *record)
{
	FILE *fp;

 	fp = fopen(XRANDERCONFIG, "w");
	if (fp == NULL)
	{
		return FALSE;
	}

	if(record->monitor1->enable == 1)
	{
		fputs(record->monitor1->port, fp);
		fputs("\n", fp);
	}
	if(record->monitor2->enable == 1)
	{
		fputs(record->monitor2->port, fp);
		fputs("\n", fp);
	}
	if(record->monitor3->enable == 1)
	{
		fputs(record->monitor3->port, fp);
		fputs("\n", fp);
	}
	
	fclose(fp);
	return TRUE;
}	

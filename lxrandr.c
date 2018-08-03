/*
 *      lxrandr.c - Easy-to-use XRandR GUI frontend for LXDE project
 *
 *      Copyright (C) 2008 Hong Jen Yee(PCMan) <pcman.tw@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <syslog.h>

#include "savexrandr.h"

#define VIRTUALRESPATH  		"/etc/virtualres.conf"
#define VIRTUALMONITOR 		"/.config/virtualmonitor.conf"
#define VIRTUALCREATEPATH	"/.config/newvirtualres.sh"
#define PREVXRANDRSHELL	"/tmp/prevnewvirtualres.sh"
#define NEXTXRANDRSHELL	"/tmp/nextnewvirtualres.sh"

#define VGAMAXRES			"1920x1280"
#define HDMIMAXRES			"1920x1280"
#define DPMAXRES			"2560x1600"

enum {
    MODE_DUPLICATE = 0,
    MODE_EXTEND,
};

enum {
    ADD_MODE_SAVE  = 1,
    ADD_MODE_APPLY,
    DEL_MODE_SAVE  ,
    DEL_MODE_APPLY,
};

typedef struct _Monitor
{
    char* name;
    GSList* mode_lines;
    short active_mode;
    char active_mode_name[20];
    short active_rate;
    short pref_mode;
    short pref_rate;
    int rotation;

    GtkCheckButton* enable;
    GtkCheckButton* plus;
#if GTK_CHECK_VERSION(2, 24, 0)
    GtkComboBoxText* res_combo;
    GtkComboBoxText* rate_combo;
    GtkComboBoxText *rotation_combo;
#else
    GtkComboBox* res_combo;
    GtkComboBox* rate_combo;
    GtkComboBox *rotation_combo;
#endif
    GtkRadioButton* primary;
	
}Monitor;

typedef struct _VirtualResolution
{
	char modeline[1024];
	char res[15];
	struct _VirtualResolution *pNext;
}VirtualRes, *pVirtualRes;

static GSList* monitors = NULL;
static int enabled = 0;
//static GSList* monitors_disconnected = NULL;
static Monitor* LVDS = NULL;
static Monitor* primary = NULL;

static GtkWidget* dlg = NULL;
static pVirtualRes vrlist;
static int iskillhdp;

/* Pointer to the used randr structure */
static xRandr *x_randr = NULL;
/* event base for XRandR notifications */
static gint randr_event_base;
static xRandrRecord *xrecord = NULL;

#if 0
#if GTK_CHECK_VERSION(2, 24, 0)
static GtkComboBoxText *rotation_combo = NULL;
#else
static GtkComboBox *rotation_combo = NULL;
#endif
#endif

#if GTK_CHECK_VERSION(2, 24, 0)
static GtkComboBoxText *multi_combo = NULL;
#else
static GtkComboBox *multi_combo = NULL;
#endif
static int multi_mode = MODE_DUPLICATE;
static char virmonpath[100];
static char vircreatetpath[100];

static char virmonpathbuf[1024];
static char vircreatetpathbuf[2048];
static int i_restart;
static GString *backupxrandr;
static char realres[2048];
static char func3buf[1024];

static GtkWidget *timedlg = NULL;
static iCountdown = 0;
static GString *prevcmd;

void init_timedialog();

//const char *virRes[]={"1024x768",
//					"1152x864"} ;

//static VirtualRes *vr = NULL;

/* Disable, not used
static void monitor_free( Monitor* m )
{
    g_free( m->name );
    g_slist_free( m->mode_lines );
    g_free( m );
}
*/

gboolean monitor_is_virtual2(char *filename, char *name, char *inresmode, char *outresmode)
{
	if(inresmode != NULL && strcmp(inresmode, "") == 0) return FALSE;
	FILE *fp;
	char getbuf[100];
	fp = fopen(filename, "r");
	if(fp == NULL)
	{
		printf("monitor_is_virtual2 open file error!\n");
		return FALSE;
	}
	char  tmpbuf[100];
	memset(tmpbuf, 0, sizeof(tmpbuf));
	if(inresmode != NULL) sprintf(tmpbuf, "%s %s", name, inresmode);
	else strcpy(tmpbuf, name);
	memset(getbuf, 0, sizeof(getbuf));
	while(fgets(getbuf, 100, fp))
	{
		if(strstr(getbuf, "#") != NULL)
		{
			char *str = strstr(getbuf, tmpbuf);
			if(str != 0)
			{
				if(outresmode != NULL) sscanf(str+strlen(name)+1, "%s", outresmode);
				fclose(fp);
				return TRUE;
			}
		}
		memset(getbuf, 0, sizeof(getbuf));
	}
	fclose(fp);
	return FALSE;
}

void monitor_mark_virtual2(char *name, char *resmode)
{
	char tmpbuf[50];
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "# %s %s\n", name, resmode);
	strcat(virmonpathbuf, tmpbuf);
}
void update_virmonpath(char *name, char *resmode)
{
		FILE *fp;
		char getbuf[100];
		char *FileBuf[5];
		int i = 0;
		fp = fopen(virmonpath, "r");
		if(fp == NULL)
		{
			printf("update_virmonpath open file error!\n");
		        return;
		}
		memset(getbuf, 0, sizeof(getbuf));
		
		while(fgets(getbuf, 100, fp))
		{
			if(strstr(getbuf, name) == NULL) 
			{
				FileBuf[i] = (char *)malloc(sizeof(char)*100);
				if(FileBuf[i] == NULL)
				{
					printf("update_virmonpath malloc error!\n");
					fclose(fp);
					return;			
				}
				memset(FileBuf[i], 0, sizeof(char)*100); 
				strcpy(FileBuf[i], getbuf);
				i++;
			}
			//else printf("DEL:%s\n", getbuf);
			memset(getbuf, 0, sizeof(getbuf));
		}
		fclose(fp);
		int j;
		fp = fopen(virmonpath, "w");
		if(fp == NULL)
		{
			printf("update_virmonpath open file error2!\n");
			for(j = 0 ; j < i; j++)
			{
				if(FileBuf[j] != NULL)
				{
					free(FileBuf[j]);
					FileBuf[j] = NULL;
				}
			}
			return;
		}	
		for(j = 0 ; j < i; j++)
		{	//printf("%s", FileBuf[j]);
		        fputs(FileBuf[j], fp);
			free(FileBuf[j]);
			FileBuf[j] = NULL;
		}
		if(resmode != NULL)
		{
			sprintf(getbuf, "# %s %s\n", name, resmode);
			fputs(getbuf, fp);
		}
		fclose(fp);
}

void update_vircreatetpath(char *name, char *resmode)
{
		FILE *fp;
		char getbuf[100];
		char *FileBuf[10];
		int i_find = 0;
		int i = 0;
		fp = fopen(vircreatetpath, "r");
		if(fp == NULL)
		{
			printf("update_vircreatetpath open file error!\n");
		        return;
		}
		memset(getbuf, 0, sizeof(getbuf));
		
		while(fgets(getbuf, 100, fp))
		{
			if(strstr(getbuf, "#") != NULL)
			{
				if(strstr(getbuf, name) != NULL && strstr(getbuf, resmode) != NULL) i_find = 1;
				else i_find = 0;
			}
			if(i_find == 0) 
			{
				FileBuf[i] = (char *)malloc(sizeof(char)*100);
				if(FileBuf[i] == NULL)
				{
					printf("update_vircreatetpath malloc error!\n");
					fclose(fp);
					return;			
				}
				memset(FileBuf[i], 0, sizeof(char)*100); 
				strcpy(FileBuf[i], getbuf);
				i++;
			}
			//else printf("DEL:%s\n", getbuf);
			memset(getbuf, 0, sizeof(getbuf));
		}
		fclose(fp);
		int j;
		fp = fopen(vircreatetpath, "w");
		if(fp == NULL)
		{
			printf("update_vircreatetpath open file error!\n");
			for(j = 0 ; j < i; j++)
			{
				if(FileBuf[j] != NULL)
				{
					free(FileBuf[j]);
					FileBuf[j] = NULL;
				}
			}
			return;
		}	
		for(j = 0 ; j < i; j++)
		{	//printf("%s", FileBuf[j]);
		        fputs(FileBuf[j], fp);
			free(FileBuf[j]);
			FileBuf[j] = NULL;
		}
		fclose(fp);
}

pVirtualRes Virtual_Res_Node_Init()
{
	pVirtualRes pNode;

	pNode = (pVirtualRes)malloc(sizeof(VirtualRes));
	memset(pNode->modeline, 0, sizeof(pNode->modeline));
	memset(pNode->res, 0, sizeof(pNode->res));
	pNode->pNext = NULL;

	return pNode;
}
void Virtual_Res_List_Add(pVirtualRes List, pVirtualRes pNode)
{
	if(List == NULL || pNode == NULL) return;
	pVirtualRes temp = List;
	while(temp->pNext != NULL)
	{
		temp = temp->pNext;
	}
	temp->pNext = pNode;
	return;
}
pVirtualRes Virtual_Res_Node_Search(pVirtualRes List, char *res)
{
	if(List == NULL || res == NULL) return NULL;

	pVirtualRes Next = List->pNext;
	
	while(Next != NULL)
	{
		if(strcmp(Next->res, res) == 0)
		{
			return Next;
		}
		Next = Next->pNext;
	}
	return NULL;
}
int  Virtual_Res_Get_Node_No(pVirtualRes List, char *res)
{
	if(List == NULL || res == NULL) return -1;

	pVirtualRes Next = List->pNext;
	int i = 0;
	while(Next != NULL)
	{
		i++;
		if(strcmp(Next->res, res) == 0)
		{
			return i;
		}
		Next = Next->pNext;
	}
	return -1;
}
void Virtual_Res_List_Free(pVirtualRes List)
{
	if(List == NULL) return;
	pVirtualRes temp = List;
	pVirtualRes Next = List->pNext;
	while(Next != NULL)
	{
		temp->pNext = Next->pNext;
		Next->pNext = NULL;
		free(Next);
		Next = temp->pNext;
	}
	if(List != NULL)
	{
		free(List);
		List = NULL;
	}
}
static gboolean init_virtual_resolution()
{
	FILE *fp;
	char getbuf[1024];

	fp = fopen(VIRTUALRESPATH, "r");
	if(fp == NULL) return FALSE;
	memset(getbuf, 0, sizeof(getbuf));
	while(fgets(getbuf, sizeof(getbuf), fp))	
	{//printf("getbuf:%s\n", getbuf);
		if(strstr(getbuf, "#") == NULL)
		{//printf("getbuf:%s\n", getbuf);
			pVirtualRes node = Virtual_Res_Node_Init();
			getbuf[strlen(getbuf)-1]='0';
			strcpy(node->modeline, getbuf);//printf("modeline:%s\n", node->modeline);
			sscanf(node->modeline+1, "%[^\"]", node->res);//printf("res:%s\n", node->res);
			Virtual_Res_List_Add(vrlist, node);
		}
		memset(getbuf, 0, sizeof(getbuf));
	}
	fclose(fp);
	return TRUE;
	//
}

#if 0
static void init_sh()
{
	char *output = NULL;
	char *err = NULL;
	char command[512];

	memset(command, 0, sizeof(command));
	sprintf(command, "cat %s", vircreatetpath);
	g_spawn_command_line_sync( command, &output, &err, NULL, NULL );
	if(err != NULL && strstr(err, "cat") != NULL)
	{
		memset(command, 0, sizeof(command));
		sprintf(command, "echo \"#! /bin/bash\" > %s", vircreatetpath);
		int ret = system(command);
		memset(command, 0, sizeof(command));
		sprintf(command, "chmod +x %s", vircreatetpath);
		ret = system(command);
	}
	g_free(output);
	g_free(err);
}
#endif
static void get_real_res()
{
#if 1
	FILE *fp;
	char tempbuf[15];
	memset(tempbuf, 0, sizeof(tempbuf));
	memset(realres, 0, sizeof(realres));
	
	if((fp = popen("cat /sys/class/drm/*/modes | uniq", "r")) == NULL)
	{
		return ;
	}
	
	while(fgets(tempbuf, sizeof(tempbuf), fp))
	{
		strcat(realres, tempbuf);
		memset(tempbuf, 0, sizeof(tempbuf));
	}
	
	pclose(fp);//printf("realres:%s\n", realres);
#endif
}
static gboolean res_is_virtual2(char *mode)
{
	if(strcmp(realres, "") != 0)
	{
		if(strstr(realres, mode) != NULL) return FALSE;
	}
	return TRUE;
}
static gboolean res_is_virtual(char *resname, char *mode)
{
	char* output = NULL;
	char command[512];
	memset(command, 0, sizeof(command));
	char Mon[20], MonNo[5];
	memset(Mon, 0, sizeof(Mon));
	memset(MonNo, 0, sizeof(MonNo));
	sscanf(resname, "%[^1-9]", Mon);//printf("Mon:%s\n", Mon);
	strcpy(MonNo, resname+strlen(Mon));//printf("MonNo:%s\n", MonNo);
	//sscanf(resname, "%[1-9]", MonNo);
	if(strcmp(Mon, "HDMI") == 0)
	{
		sprintf(command, "cat /sys/class/drm/card0-%s-A-%s/modes", Mon, MonNo);//printf("command:%s\n", command);
	}
	else
	{
		sprintf(command, "cat /sys/class/drm/card0-%s-%s/modes", Mon, MonNo);//printf("command:%s\n", command);
	}
	g_spawn_command_line_sync( command, &output, NULL, NULL, NULL );
    	if(output != NULL)
	{
		if(strstr(output, mode) != NULL) 
		{
			g_free(output);
			return FALSE;
		}
	}
	return TRUE;
}
static void virtual_resolution_func3(int action, pVirtualRes node, char *monitorname)
{
	int i_cre = 0;
	char command[1024];

	if(action == ADD_MODE_APPLY || action == ADD_MODE_SAVE)
	{
		memset(command, 0, sizeof(command));
		sprintf(command, "# %s %s\n", monitorname, node->res);
		strcat(vircreatetpathbuf, command);
		if(res_is_virtual(monitorname, node->res)) i_cre = 1;

		if(i_cre)
		{	printf("func3buf:%s_res:%s\n", func3buf, node->res);
			if(res_is_virtual2(node->res) == TRUE && strstr(func3buf, node->res) == NULL)
			{
				memset(command, 0, sizeof(command));
				sprintf(command, "xrandr --newmode %s", node->modeline);//printf("command:%s\n", command);
				if(action == ADD_MODE_APPLY) g_spawn_command_line_sync( command, NULL, NULL, NULL, NULL );
				strcat(vircreatetpathbuf, command);
				strcat(vircreatetpathbuf, "\n");
				strcpy(func3buf, command);//mark
			}

			memset(command, 0, sizeof(command));
			sprintf(command, "xrandr --addmode %s %s", monitorname, node->res);//printf("command:%s\n", command);
			if(action == ADD_MODE_APPLY) g_spawn_command_line_sync( command, NULL, NULL, NULL, NULL );
			strcat(vircreatetpathbuf, command);
			strcat(vircreatetpathbuf, "\n");
		}
	}
	else if(action == DEL_MODE_APPLY)
	{
		if(res_is_virtual(monitorname, node->res)) i_cre = 1;
		if(i_cre)
		{
			memset(command, 0, sizeof(command));
			sprintf(command, "xrandr --delmode %s %s", monitorname, node->res);//printf("command:%s\n", command);
			g_spawn_command_line_sync( command, NULL, NULL, NULL, NULL );
			//printf("func3buf:%s_res:%s\n", func3buf, node->res);
			if(res_is_virtual2(node->res) == TRUE /* && strstr(func3buf, node->res) == NULL*/)
			{
				memset(command, 0, sizeof(command));
				sprintf(command, "xrandr --rmmode %s", node->res);//printf("command:%s\n", command);
				g_spawn_command_line_sync( command, NULL, NULL, NULL, NULL );
			}
			//strcpy(func3buf, command);
		}
	}
}

static const char* get_human_readable_name( Monitor* m )
{
    if( m == LVDS )
        return _("Laptop LCD Monitor");
    else if( g_str_has_prefix( m->name, "VGA" ) || g_str_has_prefix( m->name, "Analog" ) )
        return _( LVDS ? "External VGA Monitor" : "VGA Monitor");
    else if( g_str_has_prefix( m->name, "DVI" ) || g_str_has_prefix(m->name, "TMDS") || g_str_has_prefix(m->name, "Digital") || g_str_has_prefix(m->name, "LVDS") )
        return _( LVDS ? "External DVI Monitor" : "DVI Monitor");
    else if( g_str_has_prefix( m->name, "TV" ) || g_str_has_prefix(m->name, "S-Video") )
        return _("TV");
    else if( strcmp( m->name, "default" ) == 0 )
        return _( "Default Monitor");

    return m->name;
}

static gboolean re_get_xrandr_res(Monitor *m, int res_active, int mode)//1_actual 2_virtual
{
	g_slist_free(m->mode_lines);
	m->mode_lines = NULL;

	if (mode == 1) {
	GRegex* regex;
    	GMatchInfo* match;
    	int status;
    	char* output = NULL;
	char regexbuf[1024];

    	if( ! g_spawn_command_line_sync( "/usr/bin/itep-get-xrandr-info.sh", &output, NULL, &status, NULL ) || status )
    	{
        	g_free( output );
        	return FALSE;
    	}

	memset(regexbuf, 0, sizeof(regexbuf));
	sprintf(regexbuf, "%s +connected *((([0-9]+)x([0-9]+)\\+([0-9]+)\\+([0-9]+))*) *([a-z]*) *.*(\\(.*)((\n +[0-9]+x[0-9]+[^\n]+)+)", m->name);
	char getresmode[15];
	memset(getresmode, 0, sizeof(getresmode));
	monitor_is_virtual2(virmonpath, m->name, NULL, getresmode);//printf("re___getresmode:%s\n", getresmode);
    	regex = g_regex_new( regexbuf, 0, 0, NULL );
    	if( g_regex_match( regex, output, 0, &match ) )
    	{
		do 
		{
            		char *modes = g_match_info_fetch( match, 9 );
            		char **lines, **line;
            		int imode = 0;
	    		int tmp_mark = 0;

			m->active_mode = 0;
                        m->active_rate = 0;

            		lines = g_strsplit( modes, "\n", -1 );
            		for( line = lines; *line; ++line )
            		{	
				if(strcmp(getresmode, "") != 0 && strstr(*line, getresmode) != NULL && res_is_virtual(m->name, getresmode))
				{
					//printf("getresmode is virtual:%s\n", getresmode);
					continue;
				}
				if(strstr(*line, "*") != NULL) tmp_mark = 1;
                		char* str = strtok( *line, " " );
				if(tmp_mark)
				{
					strcpy(m->active_mode_name, str);
					m->active_mode_name[strlen(m->active_mode_name)] = '\0';
					//printf("str:%s\n", m->active_mode_name);
					tmp_mark = 0;
				}
                		int irate = 0;
                		GPtrArray* strv;
                		if( ! str )
                    			continue;
                		strv = g_ptr_array_sized_new(10);
                		g_ptr_array_add( strv, g_strdup(str) );//printf("str:%s\n", str);
                		while( str = strtok( NULL, " ") )
                		{
                    			if( *str )
                    			{
                       				char *star = NULL, *plus = NULL;
                        			str = g_strdup( str );

                        			// sometimes, + goes after a space
                        			if( 0 == strcmp( str, "+" ) )
                            				--irate;
                        			else{
                            				g_ptr_array_add( strv, str );//printf("str:%s\n", str);
						}
                        			if( star = strchr( str, '*' ) )
                        			{
                            				m->active_mode = imode;
                            				m->active_rate = irate;
                        			}
                        			if( plus = strchr( str, '+' ) )
                        			{
                            				m->pref_mode = imode;
                           				m->pref_rate = irate;
                        			}
                        			if( star )
                            				*star = '\0';
                        			if( plus )
                            				*plus = '\0';
                        			++irate;
                    			}
                		}
                		g_ptr_array_add( strv, NULL );
                		m->mode_lines = g_slist_append( m->mode_lines, g_ptr_array_free( strv, FALSE ) );
                		++imode;
            		}
            		g_strfreev( lines );
            		g_free( modes );
        	}while( g_match_info_next( match, NULL ) );
        	g_match_info_free( match );
    	}
    	g_regex_unref( regex );
	g_free( output );
	}
	else if (mode == 2)
	{
		int i_find = 0;
		m->active_mode = res_active;
                m->active_rate = 0;
		m->pref_mode = 0;
                m->pref_rate = 0;
		pVirtualRes temp = vrlist->pNext;
		while(temp != NULL)
		{
			//VGA HDMI DP 
			if	   (i_find == 0
				&&strstr(m->name, "VGA") != NULL
				&&strcmp(temp->res, VGAMAXRES) == 0) 
					i_find = 1;
			else if(i_find == 0
				&&strstr(m->name, "HDMI") != NULL
				&&strcmp(temp->res, HDMIMAXRES) == 0) 
					i_find = 1;
			else if(i_find == 0
				&&strstr(m->name, "DP") != NULL 
				&& strcmp(temp->res, DPMAXRES) == 0) 
					i_find = 1;
			if(i_find == 1)
			{
				GPtrArray* strv;
				strv = g_ptr_array_sized_new(10);
		        	g_ptr_array_add( strv, temp->res );
				g_ptr_array_add( strv, "60.0" );
				g_ptr_array_add( strv, NULL );
				m->mode_lines = g_slist_append( m->mode_lines, g_ptr_array_free( strv, FALSE ) );
			}
			temp = temp->pNext;
		}
	}
    	return TRUE;
}

static gboolean get_xrandr_info()
{
    GRegex* regex;
    GMatchInfo* match;
    int status;
    char* output = NULL;
    char* ori_locale;

    ori_locale = g_strdup( setlocale(LC_ALL, "") );

    // set locale to "C" temporarily to guarantee English output of xrandr
    setlocale(LC_ALL, "C");

    if( ! g_spawn_command_line_sync( "/usr/bin/itep-get-xrandr-info.sh", &output, NULL, &status, NULL ) || status )
    {
        g_free( output );
        setlocale( LC_ALL, ori_locale );
        g_free( ori_locale );
        return FALSE;
    }

    regex = g_regex_new( "([a-zA-Z]+[-0-9]*) +connected *((([0-9]+)x([0-9]+)\\+([0-9]+)\\+([0-9]+))*) *([a-z]*) *.*(\\(.*)((\n +[0-9]+x[0-9]+[^\n]+)+)",
                         0, 0, NULL );
    if( g_regex_match( regex, output, 0, &match ) )
    {
        do {
            Monitor* m = g_new0( Monitor, 1 );
/*
		gint count = g_match_info_get_match_count( match );
		 int i;
        for ( i = 0; i < count; i++ )
        {
            gchar* word = g_match_info_fetch(match, i);
            printf("%d: %s\n",i,word);
            g_free(word);
        }
*/
	    char *xoffset = g_match_info_fetch(match, 6);
	    char *yoffset = g_match_info_fetch(match, 7);
	    char *rotation = g_match_info_fetch(match, 8);
            char *modes = g_match_info_fetch( match, 10 );
            char **lines, **line;
            int imode = 0;
	    int tmp_mark = 0;

            m->active_mode = m->active_rate = -1;
            m->pref_mode = m->pref_rate = -1;
	    memset(m->active_mode_name, 0, sizeof(m->active_mode_name));
            m->name = g_match_info_fetch( match, 1 );

	    if (strcmp(rotation, "left") == 0) {
		m->rotation = ROT_LEFT;
	    } else if (strcmp(rotation, "right") == 0) {
		m->rotation = ROT_RIGHT;
	    } else if (strcmp(rotation, "inverted") == 0) {
		m->rotation = ROT_INVERTED;
	    } else {
		m->rotation = ROT_NORMAL;
	    }

	    if (strcmp(xoffset, "0") || strcmp(yoffset, "0")) {
                multi_mode = MODE_EXTEND;
	    } else if (primary == NULL) {
                primary = m;	// primary monitor
	    }

            // check if this is the built-in LCD of laptop
            if( ! LVDS && (strcmp( m->name, "LVDS" ) == 0 || strcmp( m->name, "PANEL" ) == 0) )
                LVDS = m;

            lines = g_strsplit( modes, "\n", -1 );
            for( line = lines; *line; ++line )
            {
		if(strstr(*line, "*") != NULL) tmp_mark = 1;
                char* str = strtok( *line, " " );
		if(tmp_mark)
		{
			strcpy(m->active_mode_name, str);
			m->active_mode_name[strlen(m->active_mode_name)] = '\0';
			//printf("str:%s\n", m->active_mode_name);
			tmp_mark = 0;
		}
                int irate = 0;
                GPtrArray* strv;
                if( ! str )
                    continue;
                strv = g_ptr_array_sized_new(8);
                g_ptr_array_add( strv, g_strdup(str) );
                while( str = strtok( NULL, " ") )
                {
                    if( *str )
                    {
                        char *star = NULL, *plus = NULL;
                        str = g_strdup( str );

                        // sometimes, + goes after a space
                        if( 0 == strcmp( str, "+" ) )
                            --irate;
                        else
                            g_ptr_array_add( strv, str );

                        if( star = strchr( str, '*' ) )
                        {
                            m->active_mode = imode;
                            m->active_rate = irate;
			    enabled++;
                        }
                        if( plus = strchr( str, '+' ) )
                        {
                            m->pref_mode = imode;
                            m->pref_rate = irate;
                        }
                        if( star )
                            *star = '\0';
                        if( plus )
                            *plus = '\0';
                        ++irate;
                    }
                }
                g_ptr_array_add( strv, NULL );
                m->mode_lines = g_slist_append( m->mode_lines, g_ptr_array_free( strv, FALSE ) );
                ++imode;
            }
            if(lines != NULL) {g_strfreev( lines ); lines = NULL;}
            if(modes != NULL) {g_free( modes ); modes = NULL;}
	    if(rotation != NULL) {g_free(rotation); rotation = NULL;}
	    if(xoffset != NULL) {g_free(xoffset); xoffset = NULL;}
	    if(yoffset != NULL) {g_free(yoffset); yoffset = NULL;}
            monitors = g_slist_prepend( monitors, m );
        }while( g_match_info_next( match, NULL ) );

        if(match != NULL) {g_match_info_free( match ); match = NULL;}
    }
    g_regex_unref( regex );
#if 0
    regex = g_regex_new("([a-zA-Z]+[-0-9]*) +disconnected.*", 0, 0, NULL);
    if (g_regex_match(regex, output, 0, &match)) {
        do {
            Monitor* m = g_new0( Monitor, 1 );
	    m->name = g_match_info_fetch(match, 1);
	    monitors_disconnected = g_slist_prepend(monitors_disconnected, m);
	} while (g_match_info_next(match, NULL));
	g_match_info_free(match);
    }
    g_regex_unref(regex);
#endif
    // restore the original locale
    setlocale( LC_ALL, ori_locale );
    g_free( ori_locale );

    return TRUE;
}

static void on_res_sel_changed( GtkComboBox* cb, Monitor* m )
{
    char** rate;
    int sel = gtk_combo_box_get_active( cb );
    gchar *active_mode_name = gtk_combo_box_get_active_text( cb );
    GSList* mode_line;
#if GTK_CHECK_VERSION(2, 24, 0)
    char *res = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(cb));
    if (res == NULL) return;
    gtk_list_store_clear( GTK_LIST_STORE(gtk_combo_box_get_model( GTK_COMBO_BOX(m->rate_combo) )) );
    for (mode_line = m->mode_lines; mode_line; mode_line = mode_line->next) {
        char **strv = (char **)mode_line->data;
	if (strcmp(res, strv[0]) == 0) {
            for (rate = strv + 1; *rate; ++rate) {
                gtk_combo_box_text_append_text(m->rate_combo, *rate);
	    }
            break;
	}
    }//printf("on_res_sel_changed:sel__active_mode:%s__%s\n", active_mode_name, m->active_mode_name);
    if(strcmp(active_mode_name, m->active_mode_name) == 0)
    {
	gtk_combo_box_set_active(GTK_COMBO_BOX(m->rate_combo), m->active_rate );//printf("on_res_sel_changed:rate1-1:%d\n", m->active_rate);
    }
    else
    {
    	gtk_combo_box_set_active( GTK_COMBO_BOX(m->rate_combo), 0 );//printf("on_res_sel_changed:rate2-1:%d\n", 0);
    }

    if (multi_combo != NULL && multi_mode == MODE_DUPLICATE) {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->enable))) {
            GSList *l;
	    for (l = monitors; l; l = l->next) {
                Monitor *m1 = (Monitor *)l->data;
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m1->enable))) {
                    gtk_combo_box_set_active(GTK_COMBO_BOX(m1->res_combo), sel);
		}
	    }
	}
    }
#else
    char *res = gtk_combo_box_get_active_text(cb);
    if (res == NULL) return;
    gtk_list_store_clear( GTK_LIST_STORE(gtk_combo_box_get_model(m->rate_combo )) );
    for (mode_line = m->mode_lines; mode_line; mode_line = mode_line->next) {
        char **strv = (char **)mode_line->data;
	if (strcmp(res, strv[0]) == 0) {
            for (rate = strv + 1; *rate; ++rate) {
                gtk_combo_box_append_text(m->rate_combo, *rate);
	    }
            break;
	}
    }
    //printf("on_res_sel_changed:sel__active_mode:%s__%s\n", active_mode_name, m->active_mode_name);
    if(strcmp(active_mode_name, m->active_mode_name) == 0)
    {
	gtk_combo_box_set_active( m->rate_combo, m->active_rate );//printf("on_res_sel_changed:rate1-1:%d\n", m->active_rate);
    }
    else
    {
    	gtk_combo_box_set_active( m->rate_combo, 0 );//printf("on_res_sel_changed:rate2-1:%d\n", 0);
    }
    if (multi_combo != NULL && multi_mode == MODE_DUPLICATE) {
        if (gtk_toggle_button_get_active(m->enable)) {
            GSList *l;
	    for (l = monitors; l; l = l->next) {
                Monitor *m1 = (Monitor *)l->data;
		if (gtk_toggle_button_get_active(m1->enable)) {
                    gtk_combo_box_set_active(m1->res_combo, sel);
		}
	    }
	}
    }
#endif
	//printf("on_res_sel_changed:~~~~\n");
}

static void common_res_for_enabled(void)
{
    GSList *l, *on1 = NULL;

    for (l = monitors; l; l = l->next) {
        Monitor *m = (Monitor *)l->data;
	if(m->res_combo == NULL) continue;
        /* clear res combo */
#if GTK_CHECK_VERSION(2, 24, 0)
        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(m->res_combo))));
        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(m->rate_combo))));
#else
        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(m->res_combo)));
        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(m->rate_combo)));
#endif

        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->enable))) {
            if (on1 == NULL) {
                on1 = l;//record 
            }
        } else {
            /* set individual res for disabled monitors */
            GSList *mode_line;
            for (mode_line = m->mode_lines; mode_line; mode_line = mode_line->next) {
                char** strv = (char**)mode_line->data;
#if GTK_CHECK_VERSION(2, 24, 0)
                gtk_combo_box_text_append_text( m->res_combo, strv[0] );
#else
                gtk_combo_box_append_text( m->res_combo, strv[0] );
#endif
            }
#if GTK_CHECK_VERSION(2, 24, 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(m->res_combo), m->active_mode < 0 ? 0 : m->active_mode);
            gtk_combo_box_set_active(GTK_COMBO_BOX(m->rate_combo), m->active_rate < 0 ? 0 : m->active_rate);
#else
            gtk_combo_box_set_active(m->res_combo, m->active_mode < 0 ? 0 : m->active_mode);
            gtk_combo_box_set_active(m->rate_combo, m->active_rate < 0 ? 0 : m->active_rate);
#endif
		//printf("common_res_for_enabled1:res:%d\n", m->active_mode);
	//printf("common_res_for_enabled1:rate:%d\n", m->active_rate);
	}
    }
    Monitor *m1 = on1->data;
    GSList *mode_line;
    int i, j, active;
    gboolean active_supported = FALSE;

    for (mode_line = m1->mode_lines, i = 0, j = 0; mode_line; mode_line = mode_line->next, ++i) 
   {
        char **strv = (char **)mode_line->data;
	char *res = strv[0];
	gboolean supported = TRUE;

        /* check if this res supported by other monitors */
	for (l = on1->next; l; l = l->next) 
	{
            Monitor *m = (Monitor *)l->data;

	    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->enable))) 
	   {
		GSList *mode_line1;

		for (mode_line1 = m->mode_lines; mode_line1; mode_line1 = mode_line1->next) 
		{
			char **strv1 = (char **)mode_line1->data;
			char *res1 = strv1[0];
			if (strcmp(res, res1) == 0) break;
		}

		if (mode_line1 == NULL) 
		{
			supported = FALSE;
			break;
		}
	    }
	}

        /* set common res for enabled monitors */
	if (supported == TRUE) 
	{
            ++j;	// common res number
            for (l = on1; l; l = l->next) 
	    {
                Monitor *m = (Monitor *)l->data;
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->enable))) 
		{
#if GTK_CHECK_VERSION(2, 24, 0)
                    gtk_combo_box_text_append_text(m->res_combo, res);
#else
                    gtk_combo_box_append_text(m->res_combo, res);
#endif
		}
	    }

	    if (i == m1->active_mode) 
	   {
                active_supported = TRUE;
		active = j - 1;
	    }
	}
    }

    if (active_supported) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(m1->res_combo), active);//printf("common_res_for_enabled2:res:%d\n", active);
    } else {
	gtk_combo_box_set_active(GTK_COMBO_BOX(m1->res_combo), 0);//printf("common_res_for_enabled2:res:%d\n", 0);
    }
	
	//printf("common_res_for_enabled2:rate:%d\n", m->active_rate);
}

static void individual_res_foreach(void)
{
    GSList *l;

    for (l = monitors; l; l = l->next) {
        Monitor *m = (Monitor *)l->data;
	GSList *mode_line;
	if(m->res_combo == NULL) continue;
#if GTK_CHECK_VERSION(2, 24, 0)
        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(m->res_combo))));
        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(m->rate_combo))));
#else
        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(m->res_combo)));
        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(m->rate_combo)));
#endif

        for (mode_line = m->mode_lines; mode_line; mode_line = mode_line->next) {
            char** strv = (char**)mode_line->data;
#if GTK_CHECK_VERSION(2, 24, 0)
            gtk_combo_box_text_append_text( m->res_combo, strv[0] );
#else
            gtk_combo_box_append_text( m->res_combo, strv[0] );
#endif
        }

#if GTK_CHECK_VERSION(2, 24, 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(m->res_combo), m->active_mode < 0 ? 0 : m->active_mode);
        gtk_combo_box_set_active(GTK_COMBO_BOX(m->rate_combo), m->active_rate < 0 ? 0 : m->active_rate);
#else
        gtk_combo_box_set_active(m->res_combo, m->active_mode < 0 ? 0 : m->active_mode);
        gtk_combo_box_set_active(m->rate_combo, m->active_rate < 0 ? 0 : m->active_rate);
#endif
    }
}

static void update_res_combo(int mode)
{
    if (mode == MODE_DUPLICATE) {
        common_res_for_enabled();
    }

    if (mode == MODE_EXTEND) {
        individual_res_foreach();
    }
}

static enable_checkbox(void) {
    GSList *l;

    for (l = monitors; l; l = l->next) 
    {
        Monitor *m = (Monitor *)l->data;

	if(enabled == 1)
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->enable)))
			gtk_widget_set_sensitive(GTK_WIDGET(m->enable), FALSE);
		else
			gtk_widget_set_sensitive(GTK_WIDGET(m->enable), TRUE);
	}
	else if(enabled == 2)
	{
		if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->enable)))
			gtk_widget_set_sensitive(GTK_WIDGET(m->enable), FALSE);
		else
			gtk_widget_set_sensitive(GTK_WIDGET(m->enable), TRUE);
	}
        /*if (enabled > 1) 
	{
            gtk_widget_set_sensitive(GTK_WIDGET(m->enable), TRUE);
	}
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->enable))) 
	{
            gtk_widget_set_sensitive(GTK_WIDGET(m->enable), FALSE);
	}*/
    }
}

static void on_monitor_plus(GtkCheckButton *check, gpointer data)
{
	Monitor *m = (Monitor *)data;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check))) 
    {//add res
	char tmppath[100];
	if(i_restart == 1) strcpy(tmppath, vircreatetpath);
	else strcpy(tmppath, virmonpath);
	char getresmode[15];
	memset(getresmode, 0, sizeof(getresmode));
	if(monitor_is_virtual2(tmppath, m->name, NULL, getresmode))
	{
		int No = Virtual_Res_Get_Node_No(vrlist, getresmode);
		if(No == -1) No = 0;
		else
		{
			if(strstr(m->name, "VGA") != NULL) No = No - Virtual_Res_Get_Node_No(vrlist, VGAMAXRES);
			else if(strstr(m->name, "HDMI") != NULL) No = No - Virtual_Res_Get_Node_No(vrlist, HDMIMAXRES);
			else if(strstr(m->name, "DP") != NULL) No = No - Virtual_Res_Get_Node_No(vrlist, DPMAXRES);
		}
		re_get_xrandr_res(m, No, 2);
	}
	else re_get_xrandr_res(m, 0, 2);
    }
    else 
    {//del res
	re_get_xrandr_res(m, 0, 1);
    }
    if (multi_combo == NULL) individual_res_foreach();
    else update_res_combo(multi_mode);
}

static void on_monitor_enable(GtkCheckButton *check, gpointer data)
{
    Monitor *m = (Monitor *)data;

    if (multi_combo == NULL) return;
    update_res_combo(multi_mode);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check))) {
        enabled++;
	enable_checkbox();
        gtk_widget_set_sensitive(GTK_WIDGET(m->primary), TRUE);
	//gtk_widget_set_sensitive(GTK_WIDGET(m->plus), TRUE);
    } else {
        enabled--;
	enable_checkbox();
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->primary))) {
	    GSList *l;
	    for (l = monitors; l; l = l->next) {
	        Monitor *n = (Monitor *)l->data;
	        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(n->enable))) {
	            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(n->primary), TRUE);
	            break;
	        }
	    }
        }
        gtk_widget_set_sensitive(GTK_WIDGET(m->primary), FALSE);
	//gtk_widget_set_sensitive(GTK_WIDGET(m->plus), FALSE);
    }
}

static void on_mode_changed(GtkComboBox *cb, gpointer data)
{
    multi_mode = gtk_combo_box_get_active(cb);
    update_res_combo(multi_mode);
}

static void on_primary_changed(GtkRadioButton *radio, gpointer data)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio))) {
        primary = (Monitor *)data;
    }
}

/*Disable, not used
static void open_url( GtkDialog* dlg, const char* url, gpointer data )
{
    FIXME
}
*/

static void on_about( GtkButton* btn, gpointer parent )
{
    GtkWidget * about_dlg;
    const gchar *authors[] =
    {
        "洪任諭 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>",
        NULL
    };
    /* TRANSLATORS: Replace mw string with your names, one name per line. */
    gchar *translators = _( "translator-credits" );

//    gtk_about_dialog_set_url_hook( open_url, NULL, NULL);

    about_dlg = gtk_about_dialog_new ();

    gtk_container_set_border_width ( ( GtkContainer*)about_dlg , 2 );
    gtk_about_dialog_set_version ( (GtkAboutDialog*)about_dlg, VERSION );
    gtk_about_dialog_set_program_name ( (GtkAboutDialog*)about_dlg, _( "LXRandR" ) );
    //gtk_about_dialog_set_logo( (GtkAboutDialog*)about_dlg, gdk_pixbuf_new_from_file(  PACKAGE_DATA_DIR"/pixmaps/lxrandr.png", NULL ) );
    gtk_about_dialog_set_copyright ( (GtkAboutDialog*)about_dlg, _( "Copyright (C) 2008-2011" ) );
    gtk_about_dialog_set_comments ( (GtkAboutDialog*)about_dlg, _( "Monitor configuration tool for LXDE" ) );
    gtk_about_dialog_set_license ( (GtkAboutDialog*)about_dlg, "This program is free software; you can redistribute it and/or\nmodify it under the terms of the GNU General Public License\nas published by the Free Software Foundation; either version 2\nof the License, or (at your option) any later version.\n\nmw program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for mole details.\n\nYou should have received a copy of the GNU General Public License\nalong with mw program; if not, write to the Free Software\nFoundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA." );
    gtk_about_dialog_set_website ( (GtkAboutDialog*)about_dlg, "http://lxde.org/" );
    gtk_about_dialog_set_authors ( (GtkAboutDialog*)about_dlg, authors );
    gtk_about_dialog_set_translator_credits ( (GtkAboutDialog*)about_dlg, translators );

    gtk_window_set_transient_for( (GtkWindow*)about_dlg, (GtkWindow*)parent );
    gtk_dialog_run( ( GtkDialog*)about_dlg );
    gtk_widget_destroy( about_dlg );
}

static int 
write_xrandr_shell(char *filename, char *cmd)
{
	FILE *fp = NULL;
	fp = fopen(filename, "w");
	if(fp == NULL) 
	{
		return 0;
	}
	fputs("#!/bin/bash\n", fp);
	fputs("#lxrandr\n", fp);
	fputs(cmd, fp);
	fclose(fp);

	g_chmod (filename, 0777);
	return 1;
}

static void
get_xrandr_cmd_to_shell(char *filename)
{
	GSList* l;
    	GString *cmdon = g_string_sized_new( 500 );
	GString *cmdoff = g_string_sized_new( 500 );
	char cmd[1024];
    	char *mol = primary->name;	/* monitor on the left */	

    	g_string_assign( cmdon, "xrandr" );
	g_string_assign( cmdoff, "xrandr" );

    	for( l = monitors; l; l = l->next )
    	{
        	Monitor* m = (Monitor*)l->data;
        	// if the monitor is turned on
        	if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(m->enable) ) )
        	{
			char *resmode;
			#if GTK_CHECK_VERSION(2, 24, 0)
			resmode = gtk_combo_box_text_get_active_text(m->res_combo);
			#else
			resmode = gtk_combo_box_get_active_text(m->res_combo);
			#endif 
			g_string_append( cmdon, " --output " );
			g_string_append( cmdon, m->name );
			g_string_append_c( cmdon, ' ' );
			#if GTK_CHECK_VERSION(2, 24, 0)
	    		int sel_rotation = gtk_combo_box_get_active(GTK_COMBO_BOX(m->rotation_combo));
			#else
	    		int sel_rotation = gtk_combo_box_get_active(m->rotation_combo);
			#endif
			g_string_append( cmdon, "--mode " );
			g_string_append( cmdon, resmode );
                    	g_string_append( cmdon, " --rate " );
			#if GTK_CHECK_VERSION(2, 24, 0)
                    	g_string_append( cmdon, gtk_combo_box_text_get_active_text(m->rate_combo) );
			#else
                    	g_string_append( cmdon, gtk_combo_box_get_active_text(m->rate_combo) );
			#endif
		   	if(strcmp(m->name, primary->name) == 0)
		   	{
				g_string_append(cmdon, " --primary ");
		   	}
		    	g_string_append(cmdon, " --rotation ");
		    	switch (sel_rotation) {
		   	case 1:
				g_string_append(cmdon, "left");
				break;
		    	case 2:
				g_string_append(cmdon, "right");
				break;
		    	case 3:
				g_string_append(cmdon, "inverted");
				break;
		    	default:
				g_string_append(cmdon, "normal");
				break;
		    	}

		    	if (multi_mode == MODE_EXTEND) 
			{
				if (m != primary) 
				{
			    		g_string_append_printf(cmdon, " --right-of %s", mol);
				}
		    	} 
			else 
			{
				if (m != primary) 
				{
			    		g_string_append_printf(cmdon, " --same-as %s", mol);
				}
		    	}
		    	mol = m->name;
		    	g_string_append( cmdon, "" );
        	}
        	else    // turn off
		{
			g_string_append( cmdoff, " --output " );
			g_string_append( cmdoff, m->name );
            		g_string_append( cmdoff, " --off" );
		}
    	}
	memset(cmd, 0, sizeof(cmd));
	if(strcmp(cmdoff->str, "xrandr") != 0)
	{
		strncpy(cmd, cmdoff->str, sizeof(cmd));
		strcat(cmd, "\n");
	}
	if(strcmp(cmdon->str, "xrandr") != 0)
		strcat(cmd, cmdon->str);
	write_xrandr_shell(filename, cmd);
	g_string_free(cmdoff, TRUE);	
	g_string_free(cmdon, TRUE);
}

static GString* get_command_xrandr_info()
{
    GSList* l;
    GString *cmd = g_string_sized_new( 1024 );
    char *mol = primary->name;	/* monitor on the left */

    backupxrandr = g_string_sized_new( 1024 );
    g_string_assign( backupxrandr, "xrandr" );	

    g_string_assign( cmd, "xrandr" );

    for( l = monitors; l; l = l->next )
    {
        Monitor* m = (Monitor*)l->data;
	char *resmode;
	#if GTK_CHECK_VERSION(2, 24, 0)
	resmode = gtk_combo_box_text_get_active_text(m->res_combo);
	#else
	resmode = gtk_combo_box_get_active_text(m->res_combo);
	#endif
        g_string_append( cmd, " --output " );
        g_string_append( cmd, m->name );
        g_string_append_c( cmd, ' ' );

	g_string_append( backupxrandr, " --output " );
	g_string_append( backupxrandr, m->name );
	

        // if the monitor is turned on
        if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(m->enable) ) )
        {
	    g_string_append( backupxrandr, " --auto " );
#if GTK_CHECK_VERSION(2, 24, 0)
	    int sel_rotation = gtk_combo_box_get_active(GTK_COMBO_BOX(m->rotation_combo));
#else
	    int sel_rotation = gtk_combo_box_get_active(m->rotation_combo);
#endif

            {
                g_string_append( cmd, "--mode " );
		g_string_append( cmd, resmode );
                {
                    g_string_append( cmd, " --rate " );
#if GTK_CHECK_VERSION(2, 24, 0)
                    g_string_append( cmd, gtk_combo_box_text_get_active_text(m->rate_combo) );
#else
                    g_string_append( cmd, gtk_combo_box_get_active_text(m->rate_combo) );
#endif
                }
            }
	   if(strcmp(m->name, primary->name) == 0)
	   {
		 g_string_append(cmd, " --primary ");
	   }
	    g_string_append(cmd, " --rotation ");
	    switch (sel_rotation) {
	    case 1:
		g_string_append(cmd, "left");
		break;
	    case 2:
		g_string_append(cmd, "right");
		break;
	    case 3:
		g_string_append(cmd, "inverted");
		break;
	    default:
		g_string_append(cmd, "normal");
		break;
	    }

	    if (multi_mode == MODE_EXTEND) {
		if (m != primary) {
		    g_string_append_printf(cmd, " --right-of %s", mol);
		}
	    } else {
		if (m != primary) {
		    g_string_append_printf(cmd, " --same-as %s", mol);
		}
	    }
	    mol = m->name;

            g_string_append( cmd, "" );

        }
        else    // turn off
	{
            g_string_append( cmd, "--off" );
	    g_string_append( backupxrandr, " --off " );
	}
    }

    /*for (l = monitors_disconnected; l; l = l->next) {
        Monitor *m = (Monitor *)l->data;
        g_string_append_printf(cmd, " --output %s --off", m->name);
    }*/

    return cmd;
}

static void update_to_conf(char *filename, char *filebuf)
{
	FILE *fp = NULL;
	fp = fopen(filename, "w");
	if(fp == NULL) return;
	if(strcmp(filename, vircreatetpath) == 0)
	{
		fputs("#! /bin/bash\n", fp);
	}
	fputs(filebuf, fp);
	fclose(fp);
}

static void save_configuration(int action)
{//printf("save_configuration\n");
    GString *cmd = get_command_xrandr_info();//printf(">>>>>>cmd:%s\n", cmd->str);

    strcat(vircreatetpathbuf, "#lxrandr\n");
    //strcat(vircreatetpathbuf, backupxrandr->str);
    //strcat(vircreatetpathbuf, "\n");
    strcat(vircreatetpathbuf, cmd->str);
    update_to_conf(vircreatetpath, vircreatetpathbuf);//printf("%s\n", vircreatetpathbuf);
    char command[100];
    memset(command, 0, sizeof(command));
    sprintf(command, "chmod +x %s", vircreatetpath);
    g_spawn_command_line_sync( command, NULL, NULL, NULL, NULL );
    if(action == ADD_MODE_APPLY)update_to_conf(virmonpath, virmonpathbuf);
    if(backupxrandr != NULL) g_string_free( backupxrandr, TRUE );

    char* dirname = NULL;
    const char grp[] = "Desktop Entry";
    GKeyFile* kf;

    char* file, *data;
    gsize len;
    
    /* create user autostart dir */
    dirname = g_build_filename(g_get_user_config_dir(), "autostart", NULL);
    g_mkdir_with_parents(dirname, 0700);
    if(dirname != NULL) {g_free(dirname); dirname = NULL;}

    kf = g_key_file_new();

    g_key_file_set_string( kf, grp, "Type", "Application" );
    g_key_file_set_string( kf, grp, "Name", _("LXRandR autostart") );
    g_key_file_set_string( kf, grp, "Comment", _("Start xrandr with settings done in LXRandR") );
    g_key_file_set_string( kf, grp, "Exec",  vircreatetpath);//cmd->str
    g_key_file_set_string( kf, grp, "OnlyShowIn", "LXDE" );

    data = g_key_file_to_data(kf, &len, NULL);
    file = g_build_filename(  g_get_user_config_dir(),
                              "autostart",
                              "lxrandr-autostart.desktop",
                              NULL );
    /* save it to user-specific autostart dir */
    g_debug("save to: %s", file);
    syslog(LOG_INFO, "save to: %s", file);
    g_file_set_contents(file, data, len, NULL);
    if(kf != NULL) {g_key_file_free (kf);kf = NULL;}
    if(file != NULL) {g_free(file);file = NULL;}
    if(data != NULL) {g_free(data);data = NULL;}

}
#define MAXMODEPATH "/usr/bin/get_resolution.sh"

int GetMaxMode(char *mon, char *height, char *width)
{
	int status;
    	char* output = NULL;
	char command[1024];
	memset(command, 0, sizeof(command));
	sprintf(command, "/usr/bin/get_resolution.sh -m %s", mon);
	if( ! g_spawn_command_line_sync( command, &output, NULL, &status, NULL ) || status )
    	{
		printf("/usr/bin/get_resolution.sh failed!!!\n");
        	g_free( output );
        	return -1;
    	}
	char *p = strstr(output, "x");
	if(p == NULL)
	{	
		g_free( output );
		return -1;
	}
	//printf("q:%s\n", virRes[1]);
	//exit(1);
	strncpy(width, output, strlen(output)-strlen(p));
	strcpy(height, p+1);
	height[strlen(height)-1] = '\0';
	g_free( output );
	return 0;
}
void GetCurMode(char *cmd, char *mon, char *height, char *width)
{
	char *p = strstr(cmd, mon);
	char res[15];
	memset(res, 0, sizeof(res));
	sscanf(p+strlen(mon)+1, "--mode %s", res);printf("res:%s\n", res);
	p = strstr(res, "x");
	if(p == NULL)
	{	
		return;
	}
	strncpy(width, res, strlen(res)-strlen(p));
	strcpy(height, p+1);
	//height[strlen(height)-1] = '\0';
}

int just_for_vga_dp_to_vir2(char *dp, char *cmd, char *modename)
{
	char command[1024];
	int ret;
	
	memset(command, 0, sizeof(command));
	sprintf(command, "xrandr --output %s --mode 1024x768", dp);
	ret = system("xrandr --output VGA1 --off");
	ret = system(command);

	return 0;
}

int just_for_vga_dp_to_vir(char *dp, char *cmd, char *modename)
{
   	char vgah[10], vgaw[10], dph[10], dpw[10];
   	char command[1024];
   	char modeline[1024];//, modename[50];
   	memset(vgah, 0, sizeof(vgah));
   	memset(vgaw, 0, sizeof(vgaw));
   	memset(dph, 0, sizeof(dph));
   	memset(dpw, 0, sizeof(dpw));
   
	int status;
    	char* output = NULL;
	//get VGA and DP maxmode
	GetMaxMode(dp, dph, dpw);//printf("%s:height:%s width:%s\n", dp, dph, dpw);
	//GetCurMode(cmd, dp, dph, dpw);
	GetMaxMode("VGA1", vgah, vgaw);//printf("VGA1:height:%s width:%s\n", vgah, vgaw);
	
	memset(command, 0, sizeof(command));
	sprintf(command, "cvt %d %d", atoi(dpw)+100, atoi(dph)+100);
   	if( ! g_spawn_command_line_sync( command, &output, NULL, &status, NULL ) || status )
    	{
		printf("cvt failed!!!\n");
        	g_free( output );
        	return -1;
    	}printf("cvt command:%s\noutput:%s\n", command, output);
	char *str = strstr(output, "Modeline");
	if(str)
	{
		//memset(modeline, 0, sizeof(modeline));
		strcpy(modeline, str+strlen("Modeline")+1);
		modeline[strlen(modeline)-1] = '\0';
		sscanf(str, "%*[^\"]%s", modename);
		g_free( output );
		//printf("modeline:%s\nmodename:%s\n", modeline, modename);
	}
	else 
	{
		printf("cvt failed!!!\n");
        	g_free( output );
        	return -1;
	}
	memset(command, 0, sizeof(command));
	sprintf(command, "xrandr --newmode %s", modeline);//printf("command:%s\n", command);
	g_spawn_command_line_sync( command, NULL, NULL, NULL, NULL );
	memset(command, 0, sizeof(command));
	sprintf(command, "xrandr --addmode %s %s", dp, modename);//printf("command:%s\n", command);
	g_spawn_command_line_sync( command, NULL, NULL, NULL, NULL );
	memset(command, 0, sizeof(command));
	sprintf(command, "xrandr --output VGA1 --mode %sx%s --brightness 0  --output %s --mode %s --right-of VGA1 --brightness 0", vgaw, vgah, dp, modename);
	//printf("command:%s\n", command);
	g_spawn_command_line_sync( command, NULL, NULL, NULL, NULL );

   	return 0;
}

void just_for_vga_dp_return2(char *dp, char *modename)
{
	return;
}

void just_for_vga_dp_return(char *dp, char *modename)
{
	char command[1024];
	memset(command, 0, sizeof(command));
	sprintf(command, "xrandr --delmode %s %s", dp, modename);//printf("command:%s\n", command);
	g_spawn_command_line_sync( command, NULL, NULL, NULL, NULL );

        memset(command, 0, sizeof(command));
	sprintf(command, "xrandr --rmmode %s", modename);//printf("command:%s\n", command);
	g_spawn_command_line_sync( command, NULL, NULL, NULL, NULL );

	memset(command, 0, sizeof(command));
	sprintf(command, "xrandr --output VGA1 --brightness 1 --output %s --brightness 1", dp);//printf("command:%s\n", command);
	g_spawn_command_line_sync( command, NULL, NULL, NULL, NULL );
}

static void del_mode_xrandr(char *name)
{
	pVirtualRes node;
	char getresmode[15];
	if(name != NULL)
	{
		memset(getresmode, 0, sizeof(getresmode));
		if(monitor_is_virtual2(vircreatetpath, name, NULL, getresmode))
		{
			node = Virtual_Res_Node_Search(vrlist, getresmode);
			if(node != NULL) virtual_resolution_func3(DEL_MODE_APPLY, node, name);
			update_vircreatetpath(name, getresmode);
			memset(func3buf, 0, sizeof(func3buf));
		}
		return;
	}
	GSList* l;
	for( l = monitors; l; l = l->next )
	{
		Monitor* m = (Monitor*)l->data;
		/*if(backupxrandr == NULL)//g_string_free( backupxrandr, TRUE );
		{
			char *str = strstr(virmonpathbuf, m->name);
			if(str != NULL)
			{
				char resstr[12];
				memset(resstr, 0, sizeof(resstr));
				sscanf(str+strlen(m->name)+1, "%s", resstr);
				node = Virtual_Res_Node_Search(vrlist, resstr);
				if(node != NULL) virtual_resolution_func3(DEL_MODE_APPLY, node, m->name);
				memset(vircreatetpathbuf, 0, sizeof(vircreatetpathbuf));		
			}
		}*/
		if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->enable)) || !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->plus)))
		{
			memset(getresmode, 0, sizeof(getresmode));
			if(monitor_is_virtual2(vircreatetpath, m->name, NULL, getresmode))
			{
				node = Virtual_Res_Node_Search(vrlist, getresmode);
				if(node != NULL) virtual_resolution_func3(DEL_MODE_APPLY, node, m->name);
			}
		}
		else //if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->plus)))
		{
			char *resmode;
			#if GTK_CHECK_VERSION(2, 24, 0)
			resmode = gtk_combo_box_text_get_active_text(m->res_combo);
			#else
			resmode = gtk_combo_box_get_active_text(m->res_combo);
			#endif
			memset(getresmode, 0, sizeof(getresmode));
			if(monitor_is_virtual2(vircreatetpath, m->name, NULL, getresmode) 
			&&strcmp(getresmode, resmode) != 0)
			{
				node = Virtual_Res_Node_Search(vrlist, getresmode);
				if(node != NULL) virtual_resolution_func3(DEL_MODE_APPLY, node, m->name);
			}
		}
	}
	memset(func3buf, 0, sizeof(func3buf));
	if(backupxrandr != NULL) g_string_free( backupxrandr, TRUE );
	else 
	{
		GtkWidget *tipdlg = gtk_message_dialog_new( GTK_WINDOW(dlg),
                                      0,
                                      GTK_MESSAGE_ERROR,
                                      GTK_BUTTONS_OK,
                                      _("Monitor Don't Support the Virtual Resolution!\nMonitor has been reset."));
        	gtk_dialog_run( (GtkDialog*)tipdlg );
        	gtk_widget_destroy( tipdlg );
	}
}

static void del_virtual_xrandr(char *name, char *resmode)
{
	if(res_is_virtual(name, resmode)) 
	{
		char command[100];
		sprintf(command, "xrandr --delmode %s %s", name, resmode);
		g_spawn_command_line_sync( command, NULL, NULL, NULL, NULL );printf("del_virtual_xrandr:%s\n", command);
		if(res_is_virtual2(resmode) == TRUE )
		{
			memset(command, 0, sizeof(command));
			sprintf(command, "xrandr --rmmode %s", resmode);
			g_spawn_command_line_sync( command, NULL, NULL, NULL, NULL );printf("del_virtual_xrandr:%s\n", command);
		}
	}
}
static void get_mode_xrandr_cur(char *name1, char *name2, char *resmode1, char *resmode2)
{
	GSList* l;
	for( l = monitors; l; l = l->next )
	{
		Monitor* m = (Monitor*)l->data;

		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->enable)) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->plus)))
		{	
			if(strcmp(name1, "") == 0)
			{
				#if GTK_CHECK_VERSION(2, 24, 0)
				strcpy(resmode1, gtk_combo_box_text_get_active_text(m->res_combo));
				#else
				strcpy(resmode1, gtk_combo_box_get_active_text(m->res_combo));
				#endif
				strcpy(name1, m->name);
			}
			else
			{
				#if GTK_CHECK_VERSION(2, 24, 0)
				strcpy(resmode2, gtk_combo_box_text_get_active_text(m->res_combo));
				#else
				strcpy(resmode2, gtk_combo_box_get_active_text(m->res_combo));
				#endif
				strcpy(name2, m->name);
			}
		}
	}
}

static void new_mode_xrandr(int addaction)
{
	GSList* l;
	pVirtualRes node;
	for( l = monitors; l; l = l->next )
	{
		Monitor* m = (Monitor*)l->data;
		if(!gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(m->enable) )) continue;
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->plus)))
		{
			char *resmode;
			#if GTK_CHECK_VERSION(2, 24, 0)
			resmode = gtk_combo_box_text_get_active_text(m->res_combo);
			#else
			resmode = gtk_combo_box_get_active_text(m->res_combo);
			#endif
			if(addaction == ADD_MODE_APPLY) 
			{
				monitor_mark_virtual2(m->name, resmode);
			}
			node = Virtual_Res_Node_Search(vrlist, resmode);
			if(node != NULL) virtual_resolution_func3(addaction, node, m->name);
		}
	}
	memset(func3buf, 0, sizeof(func3buf));
}

static void set_xrandr_info(GString *cmd)
{
	char modename[50];
	memset(modename, 0, sizeof(modename));

    /*if(strstr(cmd->str, "VGA") && strstr(cmd->str, "VGA1 --off") == NULL && strstr(cmd->str, "DP1") && strstr(cmd->str, "DP1 --off") == NULL)
    {
    	if( just_for_vga_dp_to_vir2("DP1", cmd->str, modename) != 0)
	{
		g_string_free( cmd, TRUE );
		return;
	}
	sleep(2);
    }
    else if(strstr(cmd->str, "VGA") && strstr(cmd->str, "VGA1 --off") == NULL && strstr(cmd->str, "DP2") && strstr(cmd->str, "DP2 --off") == NULL)
    {
    	if( just_for_vga_dp_to_vir2("DP2", cmd->str, modename) != 0)
	{
		g_string_free( cmd, TRUE );
		return;
	}
	sleep(2);
    }*/
    //g_spawn_command_line_sync( backupxrandr->str, NULL, NULL, NULL, NULL );backupxrandr
    char *err = NULL;
    g_spawn_command_line_sync( cmd->str, NULL, &err, NULL, NULL );
    if(err != NULL && strstr(err, "failed") != NULL)
    {
	g_spawn_command_line_sync( backupxrandr->str, NULL, NULL, NULL, NULL );
	g_string_free( backupxrandr, TRUE );
	backupxrandr = NULL;
    }

    /*if(strstr(cmd->str, "VGA") && strstr(cmd->str, "VGA1 --off") == NULL && strstr(cmd->str, "DP1") && strstr(cmd->str, "DP1 --off") == NULL)
    {
    	just_for_vga_dp_return2("DP1", modename);
    }
    else if(strstr(cmd->str, "VGA") && strstr(cmd->str, "VGA1 --off") == NULL && strstr(cmd->str, "DP2") && strstr(cmd->str, "DP2 --off") == NULL)
    {
    	just_for_vga_dp_return2("DP2", modename);
    }*/
    syslog(LOG_INFO, "xrandr change display: %s", cmd->str);
    if(cmd != prevcmd) g_string_free( cmd, TRUE );
    return;
}

static void choose_max_resolution( Monitor* m )
{//printf(">>>>>>choose_max_resolution\n");
#if GTK_CHECK_VERSION(2, 24, 0)
    if( gtk_tree_model_iter_n_children( gtk_combo_box_get_model(GTK_COMBO_BOX(m->res_combo)), NULL ) > 1 )
        gtk_combo_box_set_active( GTK_COMBO_BOX(m->res_combo), 1 );
#else
    if( gtk_tree_model_iter_n_children( gtk_combo_box_get_model(m->res_combo), NULL ) > 1 )
        gtk_combo_box_set_active( m->res_combo, 1 );
#endif
}

static void on_quick_option( GtkButton* btn, gpointer data )
{
    GSList* l;
    int option = GPOINTER_TO_INT(data);
    switch( option )
    {
    case 1: // turn on both
        for( l = monitors; l; l = l->next )
        {
            Monitor* m = (Monitor*)l->data;
            choose_max_resolution( m );
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(m->enable), TRUE );
        }
        break;
    case 2: // external monitor only
        for( l = monitors; l; l = l->next )
        {
            Monitor* m = (Monitor*)l->data;
            choose_max_resolution( m );
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(m->enable), m != LVDS );
        }
        break;
    case 3: // laptop panel - LVDS only
        for( l = monitors; l; l = l->next )
        {
            Monitor* m = (Monitor*)l->data;
            choose_max_resolution( m );
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(m->enable), m == LVDS );
        }
        break;
    default:
        return;
    }
    gtk_dialog_response( GTK_DIALOG(dlg), GTK_RESPONSE_OK );
//    set_xrandr_info();
}

//全局变量
static Display *Curdisplay;
static Window Curroot;
//初始化
void initCursorDisplay()
{
     if ((Curdisplay = XOpenDisplay(NULL)) == NULL) {
         printf ("Cannot open local X-display.\n" );
         return ;
     }
     Curroot = DefaultRootWindow(Curdisplay);
} 
//设置坐标
void SetCursorPos( int x, int y)
{
     int tmp;
     XWarpPointer(Curdisplay, Curroot, Curroot, 0, 0, 0, 0, x, y);
     XFlush(Curdisplay);
} 

void
resolution_mode_to_wh(char *mode, char *w, char *h)
{
	char *strx = strstr(mode, "x");

	strncpy(w, mode, strlen(mode)-strlen(strx));
	strcpy(h, strx+1);
}

int 
xrandr_check_res(char *port, char *res)
{
	char width[10], height[10];

	memset(width, 0, sizeof(width));
	memset(height, 0, sizeof(height));
	resolution_mode_to_wh(res, width, height);
	if(strstr(port, "VGA") != NULL)
	{
		if(atoi(width) > VGAMAXWIDTH || atoi(height) > VGAMAXHEIGHT)
			return 0;
	}
	else if(strstr(port, "HDMI") != NULL)
	{
		if(atoi(width) > HDMIMAXWIDTH || atoi(height) > HDMIMAXHEIGHT)
			return 0;
	}
	else if(strstr(port, "DP") != NULL)
	{
		if(atoi(width) > DPMAXWIDTH || atoi(height) > DPMAXHEIGHT)
			return 0;
	}
	else
	{
		return 0;
	}
	return 1;
}

int check_cmd(char *cmd)
{
	char *str, *strS;
	char tmpcmd[200], port[10], res[20];

	memset(tmpcmd, 0, sizeof(tmpcmd));
	strncpy(tmpcmd, cmd, sizeof(tmpcmd));
	str = strtok_r(tmpcmd, " ", &strS);
	while(str)
	{
		if(strcmp(str, "--output") == 0)
		{
			str = strtok_r(NULL, " ", &strS);
			if(str)
			{
				memset(port, 0, sizeof(port));
				strcpy(port, str);
			}
		}
		else if(strcmp(str, "--mode") == 0)
		{
			str = strtok_r(NULL, " ", &strS);
			if(str)
			{
				memset(res, 0, sizeof(res));
				strcpy(res, str);
				if(xrandr_check_res(port, res) == 0)
				{
					return 0;
				}
			}
		}
		str = strtok_r(NULL, " ", &strS);
	}//WHILE
	return 1;
}

static void on_response( GtkDialog* dialog, int response, gpointer user_data )
{
    if( response == GTK_RESPONSE_OK )
    {
	GString *cmd = get_command_xrandr_info();
	if(check_cmd(cmd->str) == 0) 	
	{
		GtkWidget* msg = gtk_message_dialog_new( GTK_WINDOW(dialog), 0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
					     _("Out of the Max Resolution!\nVGA Max Resolution is 1920x1280;\nHDMI Max Resolution is 1920x1280;\nDP Max Resolution is 2560x1600."));
		gtk_dialog_run( GTK_DIALOG(msg) );
		gtk_widget_destroy( msg );	
		g_signal_stop_emission_by_name( dialog, "response" );
		if(cmd != NULL) 
			g_string_free( cmd, TRUE );
		return;
	}
	//printf("cmd->str:%s___ prevcmd->str:%s\n", cmd->str, prevcmd->str);
	/*if(strcmp(cmd->str, prevcmd->str) == 0)
	{
		GtkWidget *tipdlg = gtk_message_dialog_new( GTK_WINDOW(dlg),
                                      0,
                                      GTK_MESSAGE_WARNING,
                                      GTK_BUTTONS_OK,
                                      _("nothing change!."));
        	gtk_dialog_run( (GtkDialog*)tipdlg );
        	gtk_widget_destroy( tipdlg );
		return;
	}*/
	 // block the response
        //g_signal_stop_emission_by_name( dialog, "response" );

	int imon = 0;
        GSList* l;
        for( l = monitors; l; l = l->next )
        {
            Monitor* m = (Monitor*)l->data;
            if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(m->enable) ) ) imon++;
        }
	if(imon > 0 && imon < 3)
	{
		gtk_widget_set_sensitive(GTK_WIDGET(dlg), FALSE);
		iskillhdp = 0;
		FILE *fp;
		char tempbuf[150];
		memset(tempbuf, 0, sizeof(tempbuf));
		fp = popen("gksudo /usr/bin/itep_close_hdp.sh&", "r");
		if(fp != NULL && fgets(tempbuf, sizeof(tempbuf), fp))
		{
			if(strstr(tempbuf, "1") != NULL) iskillhdp = 1;
			
		}
		else
		{
			printf(" /usr/bin/itep_close_hdp.sh failed!!!\n");
		}
		if(fp != NULL)pclose(fp);
		get_xrandr_cmd_to_shell(NEXTXRANDRSHELL);
		int sysret = system(NEXTXRANDRSHELL);
		new_mode_xrandr(ADD_MODE_APPLY);
		set_xrandr_info(cmd); 
		//restart pcmanfm
		sysret = system("/usr/bin/itep_restart_pcmanfm.sh&");
		SetCursorPos( 0, 0);
		init_timedialog();
	} 
	else if(imon == 0)
	{
		 GtkWidget* msg = gtk_message_dialog_new( GTK_WINDOW(dialog), 0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
		                             _("You cannot turn off all monitors. Otherwise, you will not be able to turn them on again since this tool is not accessable without monitor.") );
		gtk_dialog_run( GTK_DIALOG(msg) );
		gtk_widget_destroy( msg );
		g_signal_stop_emission_by_name( dialog, "response" );
	}
	else 
	{
		 GtkWidget* msg = gtk_message_dialog_new( GTK_WINDOW(dialog), 0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
		                             _("The system supports at most two monitors!") );
		gtk_dialog_run( GTK_DIALOG(msg) );
		gtk_widget_destroy( msg );	
		g_signal_stop_emission_by_name( dialog, "response" );	
	}

    }
    else if (response == GTK_RESPONSE_ACCEPT)
    {
        GtkWidget* msg;
	new_mode_xrandr(ADD_MODE_SAVE);
        save_configuration(ADD_MODE_SAVE);
	//set_virtual_res(ADD_MODE_SAVE);
	//update_virtual_conf(ADD_MODE_SAVE, DEL_MODE_SAVE);
        msg = gtk_message_dialog_new( GTK_WINDOW(dialog),
                                      0,
                                      GTK_MESSAGE_INFO,
                                      GTK_BUTTONS_OK,
                                      _("Configuration Saved") );
        gtk_dialog_run( GTK_DIALOG(msg) );
        gtk_widget_destroy( msg );
    }
   /*else if(response == GTK_RESPONSE_CANCEL)
   {
           free_virtual_resolution();
   }*/
}
#if 0
int Get_Virtual_Res_List()
{
	FILE *fp;
	fp = fopen(VIRTUALRESPATH, "r");
	if(fp == NULL) return -1;
	char getbuf[1024];
	memset(getbuf, 0, sizeof(getbuf));
	while(fgets(getbuf, 1024, fp))
	{
		if(strstr(getbuf, "#") != NULL)
		{
			memset(getbuf, 0, sizeof(getbuf));
			continue;
		}
		else if(strstr(getbuf, "x") != NULL)
		{
			pVirtualRes resnode = Virtual_Res_Node_Init();
			strncpy(resnode->width, getbuf, strlen(getbuf)-strlen(strstr(getbuf, "x")));
			strcpy(resnode->height, strstr(getbuf, "x")+1);
			Virtual_Res_List_Add(vr, resnode);
		}
		memset(getbuf, 0, sizeof(getbuf));
	}
	fclose(fp);
	return 0;
}
#endif
static void get_virmonpath()
{	
	char *output = NULL;
	
	output = getenv("HOME");
	memset(virmonpath, 0, sizeof(virmonpath));
	strcpy(virmonpath, output);
	strcat(virmonpath, VIRTUALMONITOR);

	memset(vircreatetpath, 0, sizeof(vircreatetpath));
	strcpy(vircreatetpath, output);
	strcat(vircreatetpath, VIRTUALCREATEPATH);

	//printf("virmonpath:%s\n", virmonpath);
	//printf("vircreatetpath:%s\n", vircreatetpath);
}

static void 
init_shell_path(char *filename, char *path)
{	
	char *output = NULL;
	
	output = getenv("HOME");
	strcpy(filename, output);
	strcat(filename, path);
}

int main(int argc, char** argv)
{
    GtkWidget *notebook, *vbox, *frame, *label, *hbox, *check, *check2, *btn;
    GSList* l;
    int rotation = 0;

#ifdef ENABLE_NLS
	bindtextdomain ( GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR );
	bind_textdomain_codeset ( GETTEXT_PACKAGE, "UTF-8" );
	textdomain ( GETTEXT_PACKAGE );
#endif

    gtk_init( &argc, &argv );
    initCursorDisplay();

    if( ! get_xrandr_info() )
    {
        dlg = gtk_message_dialog_new( NULL,
                                      0,
                                      GTK_MESSAGE_ERROR,
                                      GTK_BUTTONS_OK,
                                      _("Unable to get monitor information!"));
        gtk_dialog_run( (GtkDialog*)dlg );
        gtk_widget_destroy( dlg );
        return 1;
    }

//20171101 get edid information by lihan
	GdkDisplay  	*display;
    	GError      	*error = NULL;
	gint         	error_base;

	/* Get the default display */
    	display = gdk_display_get_default ();

	/* Check if the randr extension is avaible on the system */
    	if (!XRRQueryExtension (gdk_x11_display_get_xdisplay (display), &randr_event_base, &error_base))
    	{
		GtkWidget* msg = gtk_message_dialog_new( NULL,
                                      0,
                                      GTK_MESSAGE_ERROR,
                                      GTK_BUTTONS_OK,
                                      _("Unable to query the version of the RandR extension being used!"));
		gtk_dialog_run( GTK_DIALOG(msg) );
		gtk_widget_destroy( msg );	

        	return 1;
    	}

	/* Create a new xrandr (>= 1.2) for this display
         * this will only work if there is 1 screen on this display */
	if (gdk_display_get_n_screens (display) == 1)
	{	
		//printf("there is one!\n");
		guint          m;	
		xRandrRecordMonitor testmonitor;

            	x_randr = xrandr_new (display, &error);
		if(!x_randr)
		{
			GtkWidget* msg = gtk_message_dialog_new( NULL,
		                              0,
		                              GTK_MESSAGE_ERROR,
		                              GTK_BUTTONS_OK,
		                              _("New xrandr error!"));
			gtk_dialog_run( GTK_DIALOG(msg) );
			gtk_widget_destroy( msg );	

			return 1;
		}			
		xrecord = xrandrrecord_new();
		for (m = 0; m < x_randr->noutput; ++m)
		{
			if(strcmp(xrecord->monitor1->manufacturer_code, "") == 0)
			{
				xrandr_get_vendor(x_randr, m, xrecord->monitor1);
			}
			else if(strcmp(xrecord->monitor2->manufacturer_code, "") == 0)
			{
				xrandr_get_vendor(x_randr, m, xrecord->monitor2);
			}
			else if(strcmp(xrecord->monitor3->manufacturer_code, "") == 0)
			{
				xrandr_get_vendor(x_randr, m, xrecord->monitor3);
			}
			/*else 
			{
				if(xrandr_get_vendor(x_randr, m, &testmonitor) == 1)
				{
					dlg = gtk_message_dialog_new( NULL,
								      0,
								      GTK_MESSAGE_ERROR,
								      GTK_BUTTONS_OK,
								      _("The system supports at most two monitors!"));
					gtk_dialog_run( (GtkDialog*)dlg );
					gtk_widget_destroy( dlg );
					return 1;
				}
			}*/
		}
	}

//end
	memset(virmonpathbuf, 0, sizeof(virmonpathbuf));
	memset(vircreatetpathbuf, 0, sizeof(vircreatetpathbuf));

	vrlist = Virtual_Res_Node_Init();
	init_virtual_resolution();
	get_virmonpath();
	get_real_res();
	memset(func3buf, 0, sizeof(func3buf));
	//init_sh();

    dlg = gtk_dialog_new_with_buttons( _("Display Settings"), NULL,
                                       GTK_DIALOG_MODAL,
                                       //GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                       GTK_STOCK_OK, GTK_RESPONSE_OK,
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL );
    g_signal_connect( dlg, "response", G_CALLBACK(on_response), NULL );
    gtk_container_set_border_width( GTK_CONTAINER(dlg), 8 );
    gtk_dialog_set_alternative_button_order( GTK_DIALOG(dlg), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1 );

    /* Set icon name for main (dlg) window so it displays in the panel. */
    gtk_window_set_icon_name(GTK_WINDOW(dlg), "display");

/* 20170715 by lh :remove about button
    btn = gtk_button_new_from_stock( GTK_STOCK_ABOUT );
#if GTK_CHECK_VERSION(2,14,0)
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area( GTK_DIALOG(dlg))), btn, FALSE, TRUE, 0 );
    gtk_button_box_set_child_secondary( GTK_BUTTON_BOX(gtk_dialog_get_action_area( GTK_DIALOG(dlg))), btn, TRUE );
#else
    gtk_box_pack_start( GTK_BOX(GTK_DIALOG(dlg)->action_area), btn, FALSE, TRUE, 0 );
    gtk_button_box_set_child_secondary( GTK_BUTTON_BOX(GTK_DIALOG(dlg)->action_area), btn, TRUE );
#endif
    g_signal_connect( btn, "clicked", G_CALLBACK(on_about), dlg );
*/
    notebook = gtk_notebook_new();
#if GTK_CHECK_VERSION(2,14,0)
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg))), notebook, TRUE, TRUE, 2 );
#else
    gtk_box_pack_start( GTK_BOX( GTK_DIALOG(dlg)->vbox ), notebook, TRUE, TRUE, 2 );
#endif

    // If this is a laptop and there is an external monitor, offer quick options
    if( LVDS && g_slist_length( monitors ) == 2 )
    {
        vbox = gtk_vbox_new( FALSE, 4 );
        gtk_container_set_border_width( GTK_CONTAINER(vbox), 8 );

        btn = gtk_button_new_with_label( _("Show the same screen on both laptop LCD and external monitor") );
        g_signal_connect( btn, "clicked", G_CALLBACK(on_quick_option), GINT_TO_POINTER(1) );
        gtk_box_pack_start( GTK_BOX(vbox), btn, FALSE, TRUE , 4);

        btn = gtk_button_new_with_label( _("Turn off laptop LCD and use external monitor only") );
        g_signal_connect( btn, "clicked", G_CALLBACK(on_quick_option), GINT_TO_POINTER(2) );
        gtk_box_pack_start( GTK_BOX(vbox), btn, FALSE, TRUE , 4);

        btn = gtk_button_new_with_label( _("Turn off external monitor and use laptop LCD only") );
        g_signal_connect( btn, "clicked", G_CALLBACK(on_quick_option), GINT_TO_POINTER(3) );
        gtk_box_pack_start( GTK_BOX(vbox), btn, FALSE, TRUE , 4);

        gtk_notebook_append_page( GTK_NOTEBOOK(notebook), vbox, gtk_label_new( _("Quick Options") ) );
    }
    else
    {
        gtk_notebook_set_show_tabs( GTK_NOTEBOOK(notebook), FALSE );
    }

    vbox = gtk_vbox_new( FALSE, 4 );
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 8 );
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), vbox, gtk_label_new(_("Advanced")) );

    label = gtk_label_new("");
    gtk_misc_set_alignment( GTK_MISC(label), 0.0, 0.5 );
    gtk_label_set_markup( GTK_LABEL(label), ngettext( "The following monitor is detected:",
                                    "The following monitors are detected:",
                                    g_slist_length(monitors) ) );
    gtk_box_pack_start( GTK_BOX(vbox), label, FALSE, TRUE, 2 );

    int i;
    GSList *group = NULL;
    for( l = monitors, i = 0; l; l = l->next, ++i )
    {
        Monitor* m = (Monitor*)l->data;
        GSList* mode_line;

        /* when primary monitor diconnected, set the first monitor as primary */
        if (primary == NULL) primary = m;

	rotation = m->rotation;

        frame = gtk_frame_new( get_human_readable_name(m) );
        gtk_box_pack_start( GTK_BOX(vbox), frame, FALSE, TRUE, 2 );

        hbox = gtk_hbox_new( FALSE, 4 );
        gtk_container_set_border_width( GTK_CONTAINER(hbox), 5 );
        gtk_container_add( GTK_CONTAINER(frame), hbox );

        check = gtk_check_button_new_with_label( _("Turn On") );
        m->enable = GTK_CHECK_BUTTON(check);

        gtk_box_pack_start( GTK_BOX(hbox), check, FALSE, TRUE, 6 );
        if( m->active_mode >= 0 )
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m->enable), TRUE);
        g_signal_connect( m->enable, "clicked", G_CALLBACK(on_monitor_enable), m );
//add resolution plus
	check2 = gtk_check_button_new_with_label( _("Plus") );
        m->plus = GTK_CHECK_BUTTON(check2);
#if 0
	gtk_box_pack_start( GTK_BOX(hbox), check2, FALSE, TRUE, 6 );
	//if( !gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(m->enable) ) )
	//	gtk_widget_set_sensitive(GTK_WIDGET(m->plus), FALSE);
	g_signal_connect( m->plus, "clicked", G_CALLBACK(on_monitor_plus), m );
#endif	
        label = gtk_label_new( _("Resolution:") );
        gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, TRUE, 2 );

#if GTK_CHECK_VERSION(2, 24, 0)
        m->res_combo = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
#else
        m->res_combo = GTK_COMBO_BOX(gtk_combo_box_new_text());
#endif
        g_signal_connect( m->res_combo, "changed", G_CALLBACK(on_res_sel_changed), m );
        gtk_box_pack_start( GTK_BOX(hbox), GTK_WIDGET(m->res_combo), FALSE, TRUE, 2 );

        label = gtk_label_new( _("Refresh Rate:") );
        gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, TRUE, 2 );

#if GTK_CHECK_VERSION(2, 24, 0)
        m->rate_combo = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
#else
        m->rate_combo = GTK_COMBO_BOX(gtk_combo_box_new_text());
#endif
	//g_signal_connect( m->rate_combo, "changed", G_CALLBACK(on_rate_sel_changed), m );
        gtk_box_pack_start( GTK_BOX(hbox), GTK_WIDGET(m->rate_combo), FALSE, TRUE, 2 );

	char getresmode[15];
	memset(getresmode, 0, sizeof(getresmode));
	//printf("m->name:%s___ m->active_mode_name:%s\n", m->name, m->active_mode_name);
	if(monitor_is_virtual2(vircreatetpath, m->name, m->active_mode_name, NULL))//vircreatetpath.virmonpath
	{//printf("1\n");
		update_virmonpath(m->name, m->active_mode_name);
		i_restart = 1;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(m->plus), TRUE );
	}
	else if (monitor_is_virtual2(virmonpath, m->name, m->active_mode_name, NULL))
	{//printf("2\n");
		i_restart = 0;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(m->plus), TRUE );
	}
	else
	{ //printf("3\n");
		i_restart = 0;
		update_virmonpath(m->name, NULL);
		if(monitor_is_virtual2(vircreatetpath, m->name, NULL, NULL))
		{//printf("in\n");
			del_mode_xrandr(m->name);
			re_get_xrandr_res(m, 0, 1);
		}
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(m->plus), FALSE );

		for( mode_line = m->mode_lines; mode_line; mode_line = mode_line->next )
		{
		    char** strv = (char**)mode_line->data;
	#if GTK_CHECK_VERSION(2, 24, 0)
		    gtk_combo_box_text_append_text( m->res_combo, strv[0] );
	#else
		    gtk_combo_box_append_text( m->res_combo, strv[0] );
	#endif
		}
	}
	#if GTK_CHECK_VERSION(2, 24, 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(m->res_combo), m->active_mode < 0 ? 0 : m->active_mode);
        gtk_combo_box_set_active(GTK_COMBO_BOX(m->rate_combo), m->active_rate < 0 ? 0 : m->active_rate);
#else
        gtk_combo_box_set_active(m->res_combo, m->active_mode < 0 ? 0 : m->active_mode);
        gtk_combo_box_set_active(m->rate_combo, m->active_rate < 0 ? 0 : m->active_rate);
#endif

	// Rotation
    	label = gtk_label_new( _("Rotation:") );
    	gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, TRUE, 2 );
#if GTK_CHECK_VERSION(2, 24, 0)
    	m->rotation_combo = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
    	gtk_combo_box_text_append_text(m->rotation_combo, _("Normal"));
    	gtk_combo_box_text_append_text(m->rotation_combo, _("Left"));
    	gtk_combo_box_text_append_text(m->rotation_combo, _("Right"));
    	gtk_combo_box_text_append_text(m->rotation_combo, _("Inverted"));
    	gtk_combo_box_set_active(GTK_COMBO_BOX(m->rotation_combo), rotation);
#else
    	m->rotation_combo = GTK_COMBO_BOX(gtk_combo_box_new_text());
    	gtk_combo_box_append_text(m->rotation_combo, _("Normal"));
    	gtk_combo_box_append_text(m->rotation_combo, _("Left"));
    	gtk_combo_box_append_text(m->rotation_combo, _("Right"));
    	gtk_combo_box_append_text(m->rotation_combo, _("Inverted"));
    	gtk_combo_box_set_active(m->rotation_combo, rotation);
#endif
    	gtk_box_pack_start( GTK_BOX(hbox), GTK_WIDGET(m->rotation_combo), FALSE, TRUE, 2 );

        m->primary = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(group, _("Primary")));
	group = gtk_radio_button_get_group(m->primary);
	if (m->active_mode < 0) gtk_widget_set_sensitive(GTK_WIDGET(m->primary), FALSE);
	g_signal_connect(m->primary, "toggled", G_CALLBACK(on_primary_changed), m);
	if (m == primary) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m->primary), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(m->primary), FALSE, TRUE, 2);
    }
    memset(func3buf, 0, sizeof(func3buf));
    hbox = gtk_hbox_new( FALSE, 4 );
    gtk_container_set_border_width( GTK_CONTAINER(hbox), 4 );

    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, TRUE, 2 );
    // Multi Monitor
    if (i > 1) {
        label = gtk_label_new( _("Multi Monitors:") );
        gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, TRUE, 2 );
#if GTK_CHECK_VERSION(2, 24, 0)
        multi_combo = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
	g_signal_connect(multi_combo, "changed", G_CALLBACK(on_mode_changed), NULL);
        gtk_combo_box_text_append_text(multi_combo, _("Duplicate these monitors"));
        gtk_combo_box_text_append_text(multi_combo, _("Extend these monitors"));
        gtk_combo_box_set_active(GTK_COMBO_BOX(multi_combo), multi_mode);
#else
        multi_combo = GTK_COMBO_BOX(gtk_combo_box_new_text());
	g_signal_connect(multi_combo, "changed", G_CALLBACK(on_mode_changed), NULL);
        gtk_combo_box_append_text(multi_combo, _("Duplicate these monitors"));
        gtk_combo_box_append_text(multi_combo, _("Extend these monitors"));
        gtk_combo_box_set_active(multi_combo, multi_mode);
#endif
        gtk_box_pack_start( GTK_BOX(hbox), GTK_WIDGET(multi_combo), FALSE, TRUE, 2 );
    }
    enable_checkbox();
    gtk_widget_show_all( dlg );
    get_xrandr_cmd_to_shell(PREVXRANDRSHELL);
    prevcmd = get_command_xrandr_info();

 /*   while(gtk_dialog_run( (GtkDialog*)dlg )  == GTK_RESPONSE_OK )
    {   
	GString *cmd = get_command_xrandr_info();
	if(strcmp(cmd->str, prevcmd->str) != 0)
	{
		set_xrandr_info(cmd); 
		init_timedialog();
	}
	if(dlg == NULL) break;
    }*/
    gtk_dialog_run( (GtkDialog*)dlg );
    if(dlg != NULL)
    {
	gtk_widget_destroy( dlg );
	dlg = NULL;
    }
    Virtual_Res_List_Free(vrlist);
	
    return 0;
}

static void on_response_timedlg( GtkDialog* dialog, int response, gpointer user_data )
{
    if( response == GTK_RESPONSE_ACCEPT && iCountdown > 0)
    {
		iCountdown = -2;
		del_mode_xrandr(NULL);
		save_configuration(ADD_MODE_APPLY);
		if(prevcmd != NULL) 
			g_string_free( prevcmd, TRUE );
		prevcmd = get_command_xrandr_info();
//20171101 get edid information by lihan
		if(parse_xrandr_command(prevcmd->str, xrecord) > 0)
		{
			update_record2(xrecord);
			Write_Monitorhotplug_Config(xrecord);
		}
    }
    else if (response ==  GTK_RESPONSE_REJECT && iCountdown > 0)
    {
		iCountdown = -2;
		char monitorname1[100],  resmode1[100];
		char monitorname2[100],  resmode2[100];

		int sysret = system(PREVXRANDRSHELL);
		//delete new virtual res
		//del_mode_xrandr_cur();
		memset(monitorname1, 0, sizeof(monitorname1));
		memset(resmode1, 0, sizeof(resmode1));
		memset(monitorname2, 0, sizeof(monitorname2));
		memset(resmode2, 0, sizeof(resmode2));
		get_mode_xrandr_cur(monitorname1, monitorname2, resmode1, resmode2);
		//get old xrandr
		//printf("cmd:%s\n", prevcmd->str);
		//if DP VGA
		//set old xrandr
		set_xrandr_info(prevcmd);
		//restart pcmanfm
		sysret = system("/usr/bin/itep_restart_pcmanfm.sh&");
		if(strcmp(monitorname1, "") != 0) del_virtual_xrandr(monitorname1, resmode1);
		if(strcmp(monitorname2, "") != 0) del_virtual_xrandr(monitorname2, resmode2);
    }
    memset(virmonpathbuf, 0, sizeof(virmonpathbuf));
    memset(vircreatetpathbuf, 0, sizeof(vircreatetpathbuf));
    sleep(1);
    gtk_widget_set_sensitive(GTK_WIDGET(dlg), TRUE);
}

gboolean countdown_function (GtkWidget *label)
{
	gboolean ret;
	char tipbuf[100];
	memset(tipbuf, 0, sizeof(tipbuf));
	--iCountdown;
	if(iCountdown > 0 || iCountdown == 0)
	{
		sprintf(tipbuf, _("Whether to save display settings?\n If not, it will restore after %d seconds"), iCountdown);
		gtk_label_set_text(GTK_LABEL(label), tipbuf);
		ret = TRUE;
		if(iCountdown == 0)
		{
			gtk_widget_set_sensitive(GTK_WIDGET(timedlg), FALSE);
		}
	}
	else if(iCountdown < 0)
	{
		if(timedlg != NULL)
		{
			gtk_widget_destroy( timedlg );
			timedlg = NULL;
		}
		if(iCountdown == -1)
		{
			char monitorname1[100],  resmode1[100];
			char monitorname2[100],  resmode2[100];
			memset(monitorname1, 0, sizeof(monitorname1));
			memset(resmode1, 0, sizeof(resmode1));
			memset(monitorname2, 0, sizeof(monitorname2));
			memset(resmode2, 0, sizeof(resmode2));
			get_mode_xrandr_cur(monitorname1, monitorname2, resmode1, resmode2);

			set_xrandr_info(prevcmd);
			//restart pcmanfm
			int sysret = system("/usr/bin/itep_restart_pcmanfm.sh&");
			if(strcmp(monitorname1, "") != 0) del_virtual_xrandr(monitorname1, resmode1);
			if(strcmp(monitorname2, "") != 0) del_virtual_xrandr(monitorname2, resmode2);
		}
		ret = FALSE;
		gtk_widget_set_sensitive(GTK_WIDGET(dlg), TRUE);
	}

	return ret;
}

void init_timedialog()
{
	timedlg = gtk_dialog_new_with_buttons( _("Display Settings"), 
					GTK_WINDOW(dlg),
                                       	GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                       	GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					GTK_STOCK_REVERT_TO_SAVED, GTK_RESPONSE_REJECT,
                                       	NULL );
    	g_signal_connect( timedlg, "response", G_CALLBACK(on_response_timedlg), NULL );
    	gtk_container_set_border_width( GTK_CONTAINER(timedlg), 8 );
    	gtk_dialog_set_alternative_button_order( GTK_DIALOG(timedlg), GTK_RESPONSE_ACCEPT, GTK_RESPONSE_REJECT, -1 );
    	gtk_window_set_icon_name(GTK_WINDOW(timedlg), "display");
	//gtk_window_move (GTK_WINDOW(timedlg), 0, 0);

	/**/
	if(iskillhdp == 1)
	{
		GtkWidget *tiplabel2 = gtk_label_new( _("Virtual Machines with apps have been closed! Restart please."));
#if GTK_CHECK_VERSION(2,14,0)
	    	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(timedlg))), tiplabel2, TRUE, TRUE, 8 );
#else
	    	gtk_box_pack_start( GTK_BOX( GTK_DIALOG(timedlg)->vbox ), tiplabel2, TRUE, TRUE, 8 );
#endif
	}

	GtkWidget *tiplabel = gtk_label_new( _("Whether to save display settings?\n If not, it will restore after 30 seconds"));
#if GTK_CHECK_VERSION(2,14,0)
    	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(timedlg))), tiplabel, TRUE, TRUE, 8 );
#else
    	gtk_box_pack_start( GTK_BOX( GTK_DIALOG(timedlg)->vbox ), tiplabel, TRUE, TRUE, 8 );
#endif
	iCountdown = 30;
	gtk_widget_set_sensitive(GTK_WIDGET(timedlg), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(dlg), FALSE);
	gtk_widget_hide(GTK_WIDGET(dlg));
	gtk_widget_show_all(timedlg);
	g_timeout_add_full(G_PRIORITY_HIGH, 1000, (GSourceFunc) countdown_function, tiplabel, NULL);
	gtk_window_move (GTK_WINDOW(timedlg), 0, 0);
	gtk_dialog_run( (GtkDialog*)timedlg );
	if(timedlg != NULL)
	{
		gtk_widget_destroy( timedlg );
		timedlg = NULL;
	}
	if (xrecord)
	{
		xrandrrecord_free(xrecord);
	}
    	if (x_randr)
        	xrandr_free (x_randr);
	if(prevcmd) 
		g_string_free( prevcmd, TRUE );
}

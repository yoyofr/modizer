
//#include "psflib.h"
#include "usf.h"


struct usf_loader_state
{
	uint32_t enable_compare;
	uint32_t enable_fifo_full;
	void * emu_state;

	usf_loader_state()
		: enable_compare(0), enable_fifo_full(0),
		  emu_state(0)
	{ }

	~usf_loader_state()
	{
		if ( emu_state )
			free( emu_state );
	}
};
int usf_loader(void * context, const uint8_t * exe, size_t exe_size, const uint8_t * reserved, size_t reserved_size)
{
	struct usf_loader_state * state = ( struct usf_loader_state * ) context;
    if ( exe_size > 0 ) 
		return -1;

	return usf_upload_section( state->emu_state, reserved, reserved_size );
}


#define MAX_UNKNOWN_TAGS			32
typedef struct {
    char lib[256];
    char libaux[8][256];
    
    char inf_title[256];
    char inf_copy[256];
    char inf_artist[256];
    char inf_game[256];
    char inf_year[256];
    char inf_length[256];
    char inf_fade[256];
    
    char inf_refresh[256];
    
    char inf_usfby[256];
    char inf_track[256];
    char inf_genre[256];
    
    char tag_name[MAX_UNKNOWN_TAGS][256];
    char tag_data[MAX_UNKNOWN_TAGS][256];
    
    uint32 *res_section;
    uint32 res_size;
} corlett_t;

corlett_t *usf_info_data;



int usf_info(void * context, const char * name, const char * value)
{
	struct usf_loader_state * state = ( struct usf_loader_state * ) context;
    
//    printf("%s %s\n",name,value);

	if ( strcasecmp( name, "_enablecompare" ) == 0 && strlen( value ) )
		state->enable_compare = 1;
	else if ( strcasecmp( name, "_enablefifofull" ) == 0 && strlen( value ) )
		state->enable_fifo_full = 1;
    
        // See if tag belongs in one of the special fields we have
        if (!strcasecmp(name, "_lib"))
        {
            strcpy(usf_info_data->lib, value);
        }
        else if (!strncmp(name, "_lib2", 5))
        {
            strcpy(usf_info_data->libaux[0], value);
        }
        else if (!strncmp(name, "_lib3", 5))
        {
            strcpy(usf_info_data->libaux[1], value);
        }
        else if (!strncmp(name, "_lib4", 5))
        {
            strcpy(usf_info_data->libaux[2], value);
        }
        else if (!strncmp(name, "_lib5", 5))
        {
            strcpy(usf_info_data->libaux[3], value);
        }
        else if (!strncmp(name, "_lib6", 5))
        {
            strcpy(usf_info_data->libaux[4], value);
        }
        else if (!strncmp(name, "_lib7", 5))
        {
            strcpy(usf_info_data->libaux[5], value);
        }
        else if (!strncmp(name, "_lib8", 5))
        {
            strcpy(usf_info_data->libaux[6], value);
        }
        else if (!strncmp(name, "_lib9", 5))
        {
            strcpy(usf_info_data->libaux[7], value);
        }
        else if (!strncmp(name, "_refresh", 8))
        {
            strcpy(usf_info_data->inf_refresh, value);
        }
        else if (!strncmp(name, "title", 5))
        {
            strcpy(usf_info_data->inf_title, value);
        }
        else if (!strncmp(name, "copyright", 9))
        {
            strcpy(usf_info_data->inf_copy, value);
        }
        else if (!strncmp(name, "artist", 6))
        {
            strcpy(usf_info_data->inf_artist, value);
        }
        else if (!strncmp(name, "game", 4))
        {
            strcpy(usf_info_data->inf_game, value);
        }
        else if (!strncmp(name, "year", 4))
        {
            strcpy(usf_info_data->inf_year, value);
        }
        else if (!strncmp(name, "length", 6))
        {
            strcpy(usf_info_data->inf_length, value);
        }
        else if (!strncmp(name, "fade", 4))
        {
            strcpy(usf_info_data->inf_fade, value);
        }
        else if (!strncmp(name, "genre", 5))
        {
            strcpy(usf_info_data->inf_genre, value);
        }
        else if (!strncmp(name, "usfby", 6))
        {
            strcpy(usf_info_data->inf_usfby, value);
        }
        else if (!strncmp(name, "track", 5))
        {
            strcpy(usf_info_data->inf_track, value);
        }

	return 0;
}

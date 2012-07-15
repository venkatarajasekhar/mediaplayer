#include <stdio.h>
#include "Utility/mp3info/mplib.h"
#include <stdlib.h>

int
main()
{
//     id3_tag_list * theList = mp_get_tag_list_from_file(
//         "/mnt/hda5/music/mp3/Whitney Houston_One Wish/01-whitney_houston-the_first_noel-xxl.mp3");
    id3_tag_list * theList = mp_get_tag_list_from_file(
        "/mnt/hda5/music/mp3/Janet Jackson-Damita Jo/01.Looking For Love.mp3");
    if (theList)
    {   
        char *array [8] = 
        {
            "",
            "artist",
            "title",
            "album",
            "genre",
            "comment",
            "year",
            "track",
        };
        
        for ( int i = 1; i < 8; ++i)
        {
            id3_content* content = mp_get_content(theList->tag, i);
            
            if (content)
            {
                id3_text_content *text_content = mp_parse_artist(content); 
                
                if (text_content)
                {
                    printf ("%s%s%s\n", array [i], ": ", text_content->text);
                    free (text_content->text);
                    free (text_content);
                }
                mp_free_content (content);
            }
        }
        mp_free_list (theList);
    }
    return 0;
}




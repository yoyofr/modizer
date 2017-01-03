//
//  ParserModland.cpp
//  modizer
//
//  Created by Yohann Magnien on 28/09/2015.
//
//

#include "ParserModland.hpp"
#include <stdlib.h>
#include <string.h>

#include "fex.h"

typedef struct {
    int file_size;
    int format_id;
    int node1_id;
    int node2_id;
    int node3_id;
    int node4_id;
    int node5_id;
    int node6_id;
    char *filename;
    void *next_entry;
} file_entry_t;

typedef struct {
    int format_id;
    char *format_name;
    void *next_entry;
} format_entry_t;

typedef struct {
    int format_id;
    char *format_name;
    void *next_entry;
} node_entry_t;

file_entry_t *files_array;
int files_array_count;

format_entry_t *formats_array;
int formats_array_count;

node_entry_t *nodes_array;
int nodes_array_count;
node_entry_t *nodes_index_label[26*2+10+1];
node_entry_t *nodes_index_label_prev[26*2+10+1];

/*
    check if node already exists, create a new one if not
 */
int parseAddNode(char *node_label) {
    //null or empty string: nothing to do
    if (!node_label) return -1;
    if (node_label[0]==0) return -2;
    
    //precompute index entry
    char c=node_label[0];
    if ((c>='a')&&(c<='z')) c=c-'a'+11+26;
    else if ((c>='A')&&(c<='Z')) c=c-'A'+11;
    else if ((c>='0')&&(c<='9')) c=c-'0'+1;
    else c=0;
    
    //existing array, search if & where to insert
    if (nodes_array) {
        //first node already exists
        node_entry_t *p=nodes_array;
        node_entry_t *prev_p=NULL;
        
        //try fast search
        if (nodes_index_label_prev[c]) p=nodes_index_label_prev[c];
        
        while (p) {
            if (strcmp(node_label,p->format_name)<0) {
                prev_p=p;
                p=(node_entry_t*)p->next_entry;
            } else if (strcmp(node_label,p->format_name)==0) {
                //duplicate
                return 0;
            } else {
                //new entry
                node_entry_t *new_entry;
                new_entry=(node_entry_t*)malloc(sizeof(node_entry_t));
                new_entry->format_name=(char*)malloc(strlen(node_label)+1);
                strcpy(new_entry->format_name,node_label);
                new_entry->next_entry=p;
                if (prev_p) prev_p->next_entry=new_entry;
                else {
                    //special case: insert at 1st place
                    nodes_array=new_entry;
                }
                
                if (nodes_index_label[c]==NULL) {
                    nodes_index_label[c]=new_entry;
                    nodes_index_label_prev[c]=prev_p;
                }/* else if (
                
                nodes_array_count++;*/
                return nodes_array_count;
            }
        }
        
    } else {
        //first node
        nodes_array=(node_entry_t*)malloc(sizeof(node_entry_t));
        nodes_array->format_name=(char*)malloc(strlen(node_label)+1);
        strcpy(nodes_array->format_name,node_label);
        nodes_array_count=1;
        
        nodes_index_label[c]=nodes_array;
        nodes_index_label_prev[c]=NULL;
    }
    return nodes_array_count;
}

int parseModland(const char *filepath) {
    FILE *f;
    
    //reset entries counters
    files_array_count=0;
    formats_array_count=0;
    nodes_array_count=0;
    memset(nodes_index_label,0,sizeof(node_entry_t)*(26*2+10+1));
    
    f=fopen(filepath,"rt");
    if (f) {
        char buffer[256];
        file_entry_t *current_file_entry;
        files_array=NULL;
        
        while (fgets(buffer,256,f)) {
            //parse a line
            //---------------
            //format: <filesize:digit><spaces><filepath>
            //sample: 1023	Ad Lib/AdLib Tracker 2 (v9 - v11)/Brendan Bailey/thee chiptune.a2m
            
            if (files_array==NULL) {
                //first entry
                current_file_entry=(file_entry_t*)malloc(sizeof(file_entry_t));
                current_file_entry->next_entry=NULL;
                files_array=current_file_entry;
            } else {
                current_file_entry->next_entry=(file_entry_t*)malloc(sizeof(file_entry_t));
                current_file_entry=(file_entry_t*)current_file_entry->next_entry;
                current_file_entry->next_entry=NULL;
            }

            int i=0;
            
            //get filesize
            int file_size=0;
            while (buffer[i]) {
                if (buffer[i]=='\t') break;
                file_size=file_size*10+(buffer[i]-'0');
                i++;
            }
            if (buffer[i]) i++;
            else {
                printf("parsing issue after file size\n");
                break;
            }
            
            current_file_entry->file_size=file_size;
            
            //get nodes & path
            char filepath[256];
            int j=0;
            int k=0;
            while (buffer[i]) {
                if (buffer[i]!='\n') {
                    if (buffer[i]=='/') {
                        //new path component
                        char node_label[256];
                        memcpy(node_label,filepath+k,j-k);
                        node_label[j-k]=0;
                        
                        if (k==0) {
                            //got 1st node: format
                            
                        } else {
                            //got 2ndary node: artist, subformat, albums, ...
                            if (parseAddNode(node_label)<0) {
                                printf("Issue when adding node: %s\n",node_label);
                            }
                        }
                        k=j+1;
                    }
                    filepath[j++]=buffer[i++];
                } else {
                    filepath[j]=0;
                    break;
                }
            }
            filepath[j]=0;
            current_file_entry->filename=(char*)malloc(j-k+1);
            memcpy(current_file_entry->filename,(const char*)&(filepath[k]),j-k);
            
//            printf("%d - %s\n",line,current_file_entry->file_size/1024,current_file_entry->filename);
            
            files_array_count++;
            printf("files: %d * nodes: %d\n",files_array_count,nodes_array_count);
        }
        printf("files: %d * nodes: %d\n",files_array_count,nodes_array_count);
    }
    fclose(f);
    
    return 0;
}

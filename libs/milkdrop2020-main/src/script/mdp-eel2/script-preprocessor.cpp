
#include "script-preprocessor.h"


namespace Script { namespace mdpx {


static std::string StripLinefeedCharsAndComments(const std::string &src)
{
    // replaces all LINEFEED_CONTROL_CHAR characters in src with a space in dest;
    // also strips out all comments (beginning with '//' and going til end of line).
    
    constexpr char LINEFEED_CONTROL_CHAR = '\n';
    
    std::string dest;
    
    int bComment = false;
    for (size_t i = 0; i<src.size(); i++)
    {
        if (bComment)
        {
            if (src[i] == LINEFEED_CONTROL_CHAR)
                bComment = false;
        }
        else
        {
            if ((src[i] == '\\' && src[i + 1] == '\\') || (src[i] == '/' && src[i + 1] == '/'))
                bComment = true;
            else if (src[i] != LINEFEED_CONTROL_CHAR)
                dest.push_back(src[i]);
        }
    }
    return dest;
}


static char *nseel_preprocessCode(char *expression)
{
    //  char *expression_start=expression;
    int len=0;
    int alloc_len=(int)strlen(expression)+1+64;
    char *buf=(char *)malloc(alloc_len);
    int semicnt=0;
    // we need to call onCompileNewLine for each new line we get
    
    //onCompileNewLine(ctx,
    
    while (*expression)
    {
        if (len > alloc_len-64)
        {
            alloc_len = len+128;
            buf=(char*)realloc(buf,alloc_len);
        }
        
        if (expression[0] == '/')
        {
            if (expression[1] == '/')
            {
                expression+=2;
                while (expression[0] && expression[0] != '\n') expression++;
                continue;
            }
            else if (expression[1] == '*')
            {
                expression+=2;
                while (expression[0] && (expression[0] != '*' || expression[1] != '/'))
                {
                    //if (expression[0] == '\n') onCompileNewLine(ctx,(int)(expression+1-expression_start),len);
                    expression++;
                }
                if (expression[0]) expression+=2; // at this point we KNOW expression[0]=* and expression[1]=/
                continue;
            }
        }
        
        if (expression[0] == '$')
        {
            if (toupper(expression[1]) == 'X')
            {
                char *p=expression+2;
                unsigned int v=(unsigned int)strtoul(expression+2,&p,16);
                char tmp[64];
                expression=p;
                
                sprintf(tmp,"%u",v);
                memcpy(buf+len,tmp,strlen(tmp));
                len+=(int)strlen(tmp);
                continue;
                
            }
            if (expression[1]=='\'' && expression[2] && expression[3]=='\'')
            {
                char tmp[64];
                sprintf(tmp,"%u",((unsigned char *)expression)[2]);
                expression+=4;
                
                memcpy(buf+len,tmp,strlen(tmp));
                len+=(int)strlen(tmp);
                continue;
            }
            if (toupper(expression[1]) == 'P' && toupper(expression[2]) == 'I')
            {
                static const char *str="3.141592653589793";
                expression+=3;
                memcpy(buf+len,str,17);
                len+=17; //strlen(str);
                continue;
            }
            if (toupper(expression[1]) == 'E')
            {
                static const char *str="2.71828183";
                expression+=2;
                memcpy(buf+len,str,10);
                len+=10; //strlen(str);
                continue;
            }
            if (toupper(expression[1]) == 'P' && toupper(expression[2]) == 'H' && toupper(expression[3]) == 'I')
            {
                static const char *str="1.61803399";
                expression+=4;
                memcpy(buf+len,str,10);
                len+=10; //strlen(str);
                continue;
            }
            
        }
        
        {
            char c=*expression++;
            
            //      if (c == '\n') onCompileNewLine(ctx,(int)(expression-expression_start),len);
            if (isspace(c)) c=' ';
            
            if (c == '(') semicnt++;
            else if (c == ')') { semicnt--; if (semicnt < 0) semicnt=0; }
            else if (c == ';' && semicnt > 0)
            {
                // convert ; to % if next nonwhitespace char is alnum, otherwise convert to space
                int p=0;
                int nc;
                int commentstate=0;
                while ((nc=expression[p]))
                {
                    if (!commentstate && nc == '/')
                    {
                        if (expression[p+1] == '/') commentstate=1;
                        else if (expression[p+1] == '*') commentstate=2;
                    }
                    
                    if (commentstate == 1 && nc == '\n') commentstate=0;
                    else if (commentstate == 2 && nc == '*' && expression[p+1]=='/')
                    {
                        p++; // skip *
                        commentstate=0;
                    }
                    else if (!commentstate && !isspace(nc)) break;
                    
                    p++;
                }
                // fucko, we should look for even more chars, me thinks
                if (nc && (isalnum(nc)
#if 1
                           || nc == '(' || nc == '_' || nc == '!' || nc == '$'
#endif
                           )) c='%';
                else c = ' '; // stray ;
            }
#if 0
            else if (semicnt > 0 && c == ',')
            {
                int p=0;
                int nc;
                while ((nc=expression[p]) && isspace(nc)) p++;
                if (nc == ',' || nc == ')')
                {
                    expression += p+1;
                    buf[len++]=',';
                    buf[len++]='0';
                    c=nc; // append this char
                }
            }
#endif
            // list of operators
            
            else if (!isspace(c) && !isalnum(c)) // check to see if this operator is ours
            {
                
                static const char *symbollists[]=
                {
                    "", // or any control char that is not parenthed
                    ":(,;?%",
                    ",):?;", // or || or &&
                    ",);", // jf> removed :? from this, for =
                    ",);",
                    "",  // only scans for a negative ] level
                    
                };
                
                
                static const struct
                {
                    char op[2];
                    char lscan,rscan;
                    const char *func;
                } preprocSymbols[] =
                {
                    {{'+','='}, 0, 3, "_addop" },
                    {{'-','='}, 0, 3, "_subop" },
                    {{'%','='}, 0, 3, "_modop" },
                    {{'|','='}, 0, 3, "_orop" },
                    {{'&','='}, 0, 3, "_andop"},
                    
                    {{'/','='}, 0, 3, "_divop"},
                    {{'*','='}, 0, 3, "_mulop"},
                    {{'^','='}, 0, 3, "_powop"},
                    
                    {{'=','='}, 1, 2, "_equal" },
                    {{'<','='}, 1, 2, "_beleq" },
                    {{'>','='}, 1, 2, "_aboeq" },
                    {{'<',0  }, 1, 2, "_below" },
                    {{'>',0  }, 1, 2, "_above" },
                    {{'!','='}, 1, 2, "_noteq" },
                    {{'|','|'}, 1, 2, "_or" },
                    {{'&','&'}, 1, 2, "_and" },
                    {{'=',0  }, 0, 3, "_set" },
                    {{'%',0},   0, 0, "_mod" },
                    {{'^',0},   0, 0, "pow" },
                    
                    
                    {{'[',0  }, 0, 5, },
                    {{'!',0  },-1, 0, }, // this should also ignore any leading +-
                    {{'?',0  }, 1, 4, },
                    
                };
                
                
                int n;
                int ns=sizeof(preprocSymbols)/sizeof(preprocSymbols[0]);
                for (n = 0; n < ns; n++)
                {
                    if (c == preprocSymbols[n].op[0] && (!preprocSymbols[n].op[1] || expression[0] == preprocSymbols[n].op[1]))
                    {
                        break;
                    }
                }
                if (n < ns)
                {
                    
                    int lscan=preprocSymbols[n].lscan;
                    int rscan=preprocSymbols[n].rscan;
                    
                    // parse left side of =, scanning back for an unparenthed nonwhitespace nonalphanumeric nonparenth?
                    // so megabuf(x+y)= would be fine, x=, but +x= would do +set(x,)
                    char *l_ptr=0;
                    char *r_ptr=0;
                    if (lscan >= 0)
                    {
                        const char *scan=symbollists[lscan];
                        int l_semicnt=0;
                        l_ptr=buf + len - 1;
                        while (l_ptr >= buf)
                        {
                            if (*l_ptr == ')') l_semicnt++;
                            else if (*l_ptr == '(')
                            {
                                l_semicnt--;
                                if (l_semicnt < 0) break;
                            }
                            else if (!l_semicnt)
                            {
                                if (!*scan)
                                {
                                    if (!isspace(*l_ptr) && !isalnum(*l_ptr) && *l_ptr != '_' && *l_ptr != '.') break;
                                }
                                else
                                {
                                    const char *sc=scan;
                                    if (lscan == 2 && ( // not currently used, even
                                                       (l_ptr[0]=='|' && l_ptr[1] == '|')||
                                                       (l_ptr[0]=='&' && l_ptr[1] == '&')
                                                       )
                                        ) break;
                                    while (*sc && *l_ptr != *sc) sc++;
                                    if (*sc) break;
                                }
                            }
                            l_ptr--;
                        }
                        buf[len]=0;
                        
                        l_ptr++;
                        
                        len = (int)(l_ptr - buf);
                        
                        l_ptr = strdup(l_ptr); // doesn't need to be preprocessed since it just was
                    }
                    if (preprocSymbols[n].op[1]) expression++;
                    
                    r_ptr=expression;
                    {
                        // scan forward to an uncommented,  unparenthed semicolon, comma, or )
                        int r_semicnt=0;
                        int r_qcnt=0;
                        const char *scan=symbollists[rscan];
                        int commentstate=0;
                        int hashadch=0;
                        int bracketcnt=0;
                        while (*r_ptr)
                        {
                            if (!commentstate && *r_ptr == '/')
                            {
                                if (r_ptr[1] == '/') commentstate=1;
                                else if (r_ptr[1] == '*') commentstate=2;
                            }
                            if (commentstate == 1 && *r_ptr == '\n') commentstate=0;
                            else if (commentstate == 2 && *r_ptr == '*' && r_ptr[1]=='/')
                            {
                                r_ptr++; // skip *
                                commentstate=0;
                            }
                            else if (!commentstate)
                            {
                                if (*r_ptr == '(') {hashadch=1; r_semicnt++; }
                                else if (*r_ptr == ')')
                                {
                                    r_semicnt--;
                                    if (r_semicnt < 0) break;
                                }
                                else if (!r_semicnt)
                                {
                                    const char *sc=scan;
                                    if (*r_ptr == ';' || *r_ptr == ',') break;
                                    
                                    if (!rscan)
                                    {
                                        if (*r_ptr == ':') break;
                                        if (!isspace(*r_ptr) && !isalnum(*r_ptr) && *r_ptr != '_' && *r_ptr != '.' && hashadch) break;
                                        if (isalnum(*r_ptr) || *r_ptr == '_')hashadch=1;
                                    }
                                    else if (rscan == 2 &&
                                             ((r_ptr[0]=='|' && r_ptr[1] == '|')||
                                              (r_ptr[0]=='&' && r_ptr[1] == '&')
                                              )
                                             ) break;
                                    
                                    else if (rscan == 3 || rscan == 4)
                                    {
                                        if (*r_ptr == ':') r_qcnt--;
                                        else if (*r_ptr == '?') r_qcnt++;
                                        
                                        if (r_qcnt < 3-rscan) break;
                                    }
                                    else if (rscan == 5)
                                    {
                                        if (*r_ptr == '[') bracketcnt++;
                                        else if (*r_ptr == ']') bracketcnt--;
                                        if (bracketcnt < 0) break;
                                    }
                                    
                                    while (*sc && *r_ptr != *sc) sc++;
                                    if (*sc) break;
                                }
                            }
                            r_ptr++;
                        }
                        // expression -> r_ptr is our string (not including r_ptr)
                        
                        {
                            char *orp=r_ptr;
                            
                            char rps=*orp;
                            *orp=0; // temporarily terminate
                            
                            r_ptr=nseel_preprocessCode(expression);
                            expression=orp;
                            
                            *orp = rps; // fix termination(restore string)
                        }
                        
                    }
                    
                    if (r_ptr)
                    {
                        int thisl = (int)strlen(l_ptr?l_ptr:"") + (int)strlen(r_ptr) + 32;
                        
                        if (len+thisl > alloc_len-64)
                        {
                            alloc_len = len+thisl+128;
                            buf=(char*)realloc(buf,alloc_len);
                        }
                        
                        
                        if (n == ns-3)
                        {
                            char *lp = l_ptr;
                            char *rp = r_ptr;
                            while (lp && *lp && isspace(*lp)) lp++;
                            while (rp && *rp && isspace(*rp)) rp++;
                            if (lp && !strncasecmp(lp,"gmem",4) && (!lp[4] || isspace(lp[4])))
                            {
                                len+=sprintf(buf+len,"_gmem(%s",r_ptr && *r_ptr ? r_ptr : "0");
                            }
                            else if (rp && *rp && strcmp(rp,"0"))
                            {
                                len+=sprintf(buf+len,"_mem((%s)+(%s)",lp,rp);
                            }
                            else
                            {
                                len+=sprintf(buf+len,"_mem(%s",lp);
                            }
                            
                            // skip the ]
                            if (*expression == ']') expression++;
                            
                        }
                        else if (n == ns-2)
                        {
                            len+=sprintf(buf+len,"_not(%s",
                                         r_ptr);
                            
                        }
                        else if (n == ns-1)// if (l_ptr,r_ptr1,r_ptr2)
                        {
                            char *rptr2=r_ptr;
                            char *tmp=r_ptr;
                            int parcnt=0;
                            int qcnt=1;
                            while (*rptr2)
                            {
                                if (*rptr2 == '?') qcnt++;
                                else if (*rptr2 == ':') qcnt--;
                                else if (*rptr2 == '(') parcnt++;
                                else if (*rptr2 == ')') parcnt--;
                                if (parcnt < 0) break;
                                if (!parcnt && !qcnt && *rptr2 == ':') break;
                                rptr2++;
                            }
                            if (*rptr2) *rptr2++=0;
                            while (isspace(*rptr2)) rptr2++;
                            
                            while (isspace(*tmp)) tmp++;
                            
                            len+=sprintf(buf+len,"_if(%s,%s,%s",l_ptr,*tmp?tmp:"0",*rptr2?rptr2:"0");
                        }
                        else
                        {
                            len+=sprintf(buf+len,"%s(%s,%s",preprocSymbols[n].func,l_ptr?l_ptr:"",r_ptr);
                        }
                        
                    }
                    
                    free(r_ptr);
                    free(l_ptr);
                    
                    
                    c = ')'; // close parenth below
                }
            }
            
            //      if (c != ' ' || (len && buf[len-1] != ' ')) // don't bother adding multiple spaces
            {
                buf[len++]=c;
            }
        }
    }
    buf[len]=0;
    
    return buf;
}

static std::string nseel_preprocessCode_safe(const std::string &code)
{
    char *code_copy = strdup(code.c_str());
    char *code_result = nseel_preprocessCode(code_copy);
    
    // return as result
    std::string result(code_result);
    
    // free copies
    free(code_result);
    free(code_copy);
    return result;
}

void nseel_preprocessCode(const std::string &code, std::vector<std::string> &lines)
{
    if (code.empty())
        return;
    
    std::string stripped = StripLinefeedCharsAndComments(code);
    if (stripped.empty())
        return;
    
    std::string preprocessed_code =nseel_preprocessCode_safe(stripped);
//    std::cout << code << '\n';
//    std::cout << preprocessed_code << '\n';
    
    const char *expression = preprocessed_code.c_str();
    while (*expression)
    {
        // single out segment
        while (*expression == ';' || isspace(*expression)) expression++;
        if (!*expression) break;
        
        std::string expr;
        while (*expression && *expression != ';') {
            expr.push_back(*expression++);
        }
        
        lines.push_back(expr);
    }
}



}}

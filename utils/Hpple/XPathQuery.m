//
//  XPathQuery.m
//  FuelFinder
//
//  Created by Matt Gallagher on 4/08/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "XPathQuery.h"

#import <libxml/tree.h>
#import <libxml/parser.h>
#import <libxml/HTMLparser.h>
#import <libxml/xpath.h>
#import <libxml/xpathInternals.h>

NSDictionary *DictionaryForNode(xmlNodePtr currentNode, NSMutableDictionary *parentResult,BOOL parentContent);
NSArray *PerformXPathQuery(xmlDocPtr doc, NSString *query);

NSDictionary *DictionaryForNode(xmlNodePtr currentNode, NSMutableDictionary *parentResult,BOOL parentContent)
{
    NSMutableDictionary *resultForNode = [NSMutableDictionary dictionary];
    
//    NSLog(@"new node");
    
    if (currentNode->name) {
        NSString *currentNodeContent = [NSString stringWithCString:(const char *)currentNode->name
                                                          encoding:NSUTF8StringEncoding];
        resultForNode[@"nodeName"] = currentNodeContent;
//        NSLog(@"name: %s",(const char*)currentNode->name);
    }

    xmlChar *nodeContent = xmlNodeGetContent(currentNode);
    if (nodeContent != NULL) {
        NSString *currentNodeContent = [NSString stringWithCString:(const char *)nodeContent
                                                          encoding:NSUTF8StringEncoding];
//        NSLog(@"content: %s",(const char*)nodeContent);
        
        if ([resultForNode[@"nodeName"] isEqual:@"text"] && parentResult) {
            if (parentContent) {
                NSCharacterSet *charactersToTrim = [NSCharacterSet whitespaceAndNewlineCharacterSet];
                parentResult[@"nodeContent"] = [currentNodeContent stringByTrimmingCharactersInSet:charactersToTrim];
                /** Memory leak point release, Prevent memory leak */
                xmlFree(nodeContent);
                /** Memory leak point release, Prevent memory leak */
                return nil;
            }
            if (currentNodeContent != nil) {
                resultForNode[@"nodeContent"] = currentNodeContent;
            }
            /** Memory leak point release, Prevent memory leak */
            xmlFree(nodeContent);
            /** Memory leak point release, Prevent memory leak */
            return resultForNode;
        } else {
            resultForNode[@"nodeContent"] = currentNodeContent;
        }
        xmlFree(nodeContent);
    }

    xmlAttr *attribute = currentNode->properties;
    if (attribute) {
        NSMutableArray *attributeArray = [NSMutableArray array];
        while (attribute) {
            NSMutableDictionary *attributeDictionary = [NSMutableDictionary dictionary];
            NSString *attributeName = [NSString stringWithCString:(const char *)attribute->name
                                                       encoding:NSUTF8StringEncoding];
            if (attributeName) {
                attributeDictionary[@"attributeName"] = attributeName;
            }
          
            if (attribute->children) {
                NSDictionary *childDictionary = DictionaryForNode(attribute->children, attributeDictionary, true);
                if (childDictionary) {
                    attributeDictionary[@"attributeContent"] = childDictionary;
                }
            }

            if ([attributeDictionary count] > 0) {
                [attributeArray addObject:attributeDictionary];
            }
            attribute = attribute->next;
        }

        if ([attributeArray count] > 0) {
            resultForNode[@"nodeAttributeArray"] = attributeArray;
        }
    }

    xmlNodePtr childNode = currentNode->children;
    if (childNode) {
        NSMutableArray *childContentArray = [NSMutableArray array];
        while (childNode) {
            NSDictionary *childDictionary = DictionaryForNode(childNode, resultForNode,false);
            if (childDictionary) {
                [childContentArray addObject:childDictionary];
            }
            childNode = childNode->next;
        }
        if ([childContentArray count] > 0) {
            resultForNode[@"nodeChildArray"] = childContentArray;
        }
    }

    xmlBufferPtr buffer = xmlBufferCreate();
    xmlNodeDump(buffer, currentNode->doc, currentNode, 0, 0);
    NSString *rawContent = [NSString stringWithCString:(const char *)buffer->content encoding:NSUTF8StringEncoding];
    if (rawContent != nil) {
        resultForNode[@"raw"] = rawContent;
    }
    xmlBufferFree(buffer);
    return resultForNode;
}


/**
 * print_element_names:
 * @a_node: the initial xml node to consider.
 *
 * Prints the names of the all the xml elements
 * that are siblings or children of a given xml node.
 */
static void
print_element_names(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            xmlChar *nodeContent = xmlNodeGetContent(cur_node);
            printf("node type: Element, name: %s, content: %s\n", cur_node->name,nodeContent);
            if (nodeContent) xmlFree(nodeContent);
        }

        print_element_names(cur_node->children);
    }
}

NSArray *PerformXPathQuery(xmlDocPtr doc, NSString *query)
{
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;
    
    
//    xmlNode *root_element = NULL;
//    root_element=xmlDocGetRootElement(doc);
//    print_element_names(root_element);

    /* Make sure that passed query is non-nil and is NSString object */
    if (query == nil || ![query isKindOfClass:[NSString class]]) {
        return nil;
    }
  
    /* Create xpath evaluation context */
    xpathCtx = xmlXPathNewContext(doc);
    if(xpathCtx == NULL) {
        NSLog(@"Unable to create XPath context.");
        return nil;
    }

    /* Evaluate xpath expression */
    xpathObj = xmlXPathEvalExpression((xmlChar *)[query cStringUsingEncoding:NSUTF8StringEncoding], xpathCtx);
    if(xpathObj == NULL) {
        NSLog(@"Unable to evaluate XPath.");
        xmlXPathFreeContext(xpathCtx);
        return nil;
    }

    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    if (!nodes) {
        NSLog(@"Nodes was nil.");
        xmlXPathFreeObject(xpathObj);
        xmlXPathFreeContext(xpathCtx);
        return nil;
    }

    NSMutableArray *resultNodes = [NSMutableArray array];
    for (NSInteger i = 0; i < nodes->nodeNr; i++) {
        NSDictionary *nodeDictionary = DictionaryForNode(nodes->nodeTab[i], nil,false);
        if (nodeDictionary) {
            [resultNodes addObject:nodeDictionary];
        }
    }

    /* Cleanup */
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);

    return resultNodes;
}

NSArray *PerformHTMLXPathQuery(NSData *document, NSString *query) {
    return PerformHTMLXPathQueryWithEncoding(document, query, nil);
}

NSArray *PerformHTMLXPathQueryWithEncoding(NSData *document, NSString *query,NSString *encoding)
{
    xmlDocPtr doc;

    /* Load XML document */
    const char *encoded = encoding ? [encoding cStringUsingEncoding:NSUTF8StringEncoding] : "UTF-8";//NULL;
    
//    NSLog(@"query: %@",query);

    //doc = htmlReadMemory([document bytes], (int)[document length], "", encoded, HTML_PARSE_NOWARNING | HTML_PARSE_NOERROR);
    doc = htmlReadMemory([document bytes], (int)[document length], "", encoded, HTML_PARSE_RECOVER|HTML_PARSE_NOWARNING | HTML_PARSE_NOERROR);
    if (doc == NULL) {
        NSLog(@"Unable to parse.");
        return nil;
    }
    
    NSArray *result = PerformXPathQuery(doc, query);
    
//    if (result) NSLog(@"Got: array of %d elements",[result count]);
//    else NSLog(@"Got: null array!");
    
    xmlFreeDoc(doc);
    
    return result;
}

NSArray *PerformXMLXPathQuery(NSData *document, NSString *query) {
    return PerformXMLXPathQueryWithEncoding(document, query, nil);
}

NSArray *PerformXMLXPathQueryWithEncoding(NSData *document, NSString *query,NSString *encoding)
{
    xmlDocPtr doc;
    
    /* Load XML document */
    const char *encoded = encoding ? [encoding cStringUsingEncoding:NSUTF8StringEncoding] : NULL;

    doc = xmlReadMemory([document bytes], (int)[document length], "", encoded, XML_PARSE_RECOVER);
    
    if (doc == NULL) {
        NSLog(@"Unable to parse.");
        return nil;
    }
    
    NSArray *result = PerformXPathQuery(doc, query);
    xmlFreeDoc(doc);
    
    return result;
}

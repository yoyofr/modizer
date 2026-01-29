//
//  StringMatcher.h
//  modizer
//
//  Created by Yohann Magnien David on 28/01/2026.
//

#import <Foundation/Foundation.h>

@interface StringMatcher : NSObject

/**
 * Trouve l'index du meilleur match dans un tableau de chaînes
 * @param search La chaîne à rechercher
 * @param array Le tableau de candidats
 * @param minimumScore Le score minimum requis (0.0 à 1.0)
 * @return L'index du meilleur match, ou -1 si aucun ne dépasse le score minimum
 */
- (NSInteger)bestMatchIndexFor:(NSString *)search
                       inArray:(NSArray<NSString *> *)array
                  minimumScore:(CGFloat)minimumScore;

/**
 * Calcule le score de similarité entre deux chaînes
 * @param candidate La chaîne candidate
 * @param search La chaîne recherchée
 * @return Un score entre 0.0 et ~1.2
 */
- (CGFloat)scoreForMatch:(NSString *)candidate withSearch:(NSString *)search;

@end

//
//  ModizerAppIntents.swift
//  modizer
//
//  Created by Yohann Magnien David on 30/11/2025.
//

import AppIntents
import Foundation
import UIKit

// MARK: - Player Bridge
/// Bridge to access Modizer player functionality from Swift
@available(iOS 16.0, *)
@MainActor
class ModizerPlayerBridge {
    static let shared = ModizerPlayerBridge()

    private init() {}

    /// Check if the app is properly initialized
    private var isAppInitialized: Bool {
        guard let appDelegate = UIApplication.shared.delegate as? NSObject else {
            return false
        }
        // Check if key view controllers are set up
        return appDelegate.value(forKey: "playlistVC") != nil &&
               appDelegate.value(forKey: "detailViewControlleriPhone") != nil
    }

    // Get the detail view controller from app delegate
    private var detailViewController: AnyObject? {
        guard let appDelegate = UIApplication.shared.delegate as? NSObject,
              let detailVC = appDelegate.value(forKey: "detailViewControlleriPhone") as? NSObject else {
            return nil
        }
        return detailVC
    }
    
    private var tabBarController: AnyObject? {
        guard let appDelegate = UIApplication.shared.delegate as? NSObject,
              let tabBarC = appDelegate.value(forKey: "tabBarC") as? NSObject else {
            return nil
        }
        return tabBarC
    }

    private var musicPlayer: AnyObject? {
        guard let detailVC = detailViewController as? NSObject,
              let player = detailVC.value(forKey: "mplayer") as? NSObject else {
            return nil
        }
        return player
    }

    func play() {
        detailViewController?.perform(NSSelectorFromString("playPushed"))
    }

    func pause() {
        detailViewController?.perform(NSSelectorFromString("pausePushed"))
    }

    func stop() {
        musicPlayer?.perform(NSSelectorFromString("Stop"))
    }

    func playNextSong() {
        detailViewController?.perform(NSSelectorFromString("play_nextEntry"))
    }

    func playPreviousSong() {
        detailViewController?.perform(NSSelectorFromString("play_prevEntry"))
    }

    func playNextSubsong() {
        musicPlayer?.perform(NSSelectorFromString("playNextSub"))
    }

    func playPreviousSubsong() {
        musicPlayer?.perform(NSSelectorFromString("playPrevSub"))
    }

    func playSubsong(index: Int) {
        guard let player = musicPlayer as? NSObject else { return }

        let selector = NSSelectorFromString("playGoToSub:")
        guard player.responds(to: selector) else { return }

        let method = player.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector, Int32) -> Void
        let implementation = unsafeBitCast(method, to: MethodType.self)
        implementation(player, selector, Int32(index))
    }

    func seek(to position: Int64) {
        guard let player = musicPlayer as? NSObject else { return }

        let selector = NSSelectorFromString("Seek:")
        guard player.responds(to: selector) else { return }

        let method = player.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector, Int64) -> Void
        let implementation = unsafeBitCast(method, to: MethodType.self)
        implementation(player, selector, position)
    }

    func getCurrentTime() -> Int {
        guard let player = musicPlayer as? NSObject else {
            return 0
        }

        let selector = NSSelectorFromString("getCurrentTime")
        guard player.responds(to: selector) else { return 0 }

        let method = player.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector) -> Int32
        let implementation = unsafeBitCast(method, to: MethodType.self)
        let result = implementation(player, selector)

        return Int(result/1000)
    }

    func getSongLength() -> Int {
        guard let player = musicPlayer as? NSObject else {
            return 0
        }

        let selector = NSSelectorFromString("getSongLength")
        guard player.responds(to: selector) else { return 0 }

        let method = player.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector) -> Int32
        let implementation = unsafeBitCast(method, to: MethodType.self)
        let result = implementation(player, selector)

        return Int(result/1000)
    }

    func getCurrentSongTitle() -> String {
        guard let player = musicPlayer as? NSObject else {
            return "Unknown"
        }

        let selector = NSSelectorFromString("getModFileTitle")
        guard player.responds(to: selector) else { return "Unknown" }

        let method = player.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector) -> Unmanaged<AnyObject>?
        let implementation = unsafeBitCast(method, to: MethodType.self)

        guard let result = implementation(player, selector)?.takeUnretainedValue() as? String else {
            return "Unknown"
        }

        return result
    }
    
    func getCurrentSongAlbum() -> String {
        guard let player = musicPlayer as? NSObject else {
            return "Unknown"
        }

        let selector = NSSelectorFromString("getModName")
        guard player.responds(to: selector) else { return "Unknown" }

        let method = player.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector) -> Unmanaged<AnyObject>?
        let implementation = unsafeBitCast(method, to: MethodType.self)

        guard let result = implementation(player, selector)?.takeUnretainedValue() as? String else {
            return "Unknown"
        }

        return result
    }
    
    func getCurrentSongArtist() -> String {
        guard let player = musicPlayer as? NSObject else {
            return "Unknown"
        }

        let selector = NSSelectorFromString("artist")
        guard player.responds(to: selector) else { return "Unknown" }

        let method = player.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector) -> Unmanaged<AnyObject>?
        let implementation = unsafeBitCast(method, to: MethodType.self)

        guard let result = implementation(player, selector)?.takeUnretainedValue() as? String else {
            return "Unknown"
        }

        return result
    }

    func isPlaying() -> Bool {
        guard let player = musicPlayer as? NSObject else {
            return false
        }

        let selector = NSSelectorFromString("isPlaying")
        guard player.responds(to: selector) else { return false }

        let method = player.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector) -> Bool
        let implementation = unsafeBitCast(method, to: MethodType.self)
        let result = implementation(player, selector)

        return result
    }

    func setInfiniteLoopMode(_ mode: Int) {
        guard let detailVC = detailViewController as? NSObject else { return }

        let selector = NSSelectorFromString("setLoopInf:")
        guard detailVC.responds(to: selector) else { return }

        let method = detailVC.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector, Int32) -> Void
        let implementation = unsafeBitCast(method, to: MethodType.self)
        implementation(detailVC, selector, Int32(mode))
    }

    func setLoopMode(_ mode: Int) {
        guard let detailVC = detailViewController as? NSObject else { return }

        let selector = NSSelectorFromString("setLoopMode:")
        guard detailVC.responds(to: selector) else { return }

        let method = detailVC.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector, Int32) -> Void
        let implementation = unsafeBitCast(method, to: MethodType.self)
        implementation(detailVC, selector, Int32(mode))
    }
    
    func setFX(_ fxIdx: Int, value: Int) {
        guard let detailVC = detailViewController as? NSObject else { return }

        let selector = NSSelectorFromString("mdSetFX:value:")
        guard detailVC.responds(to: selector) else { return }

        let method = detailVC.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector, Int32, Int32) -> Void
        let implementation = unsafeBitCast(method, to: MethodType.self)
        implementation(detailVC, selector, Int32(fxIdx), Int32(value))
    }
    
    func goToPlayerView() {
        tabBarController?.perform(NSSelectorFromString("goToPlayerView"))
    }
    
    func toggleLoopMode() {
        detailViewController?.perform(NSSelectorFromString("changeLoopMode"))
    }
    
    func toggleShuffle() {
        detailViewController?.perform(NSSelectorFromString("shuffle"))
    }
    
    func setShuffleMode(_ mode: Int) {
        guard let detailVC = detailViewController as? NSObject else { return }

        let selector = NSSelectorFromString("setShuffleMode:")
        guard detailVC.responds(to: selector) else { return }

        let method = detailVC.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector, Int32) -> Void
        let implementation = unsafeBitCast(method, to: MethodType.self)
        implementation(detailVC, selector, Int32(mode))
    }

    func getShuffleState() -> Int {
        guard let detailVC = detailViewController as? NSObject,
              let shuffleState = detailVC.value(forKey: "mShuffle") as? Int32 else {
            return 0
        }
        return Int(shuffleState)
    }

    // MARK: - Playlist Methods

    func getAvailablePlaylists() -> [(id: Int, name: String, size: Int)] {
        // Use runtime lookup to access ModizerPlaylistBridge
        guard let bridgeClass = NSClassFromString("ModizerPlaylistBridge") as? NSObject.Type else {
            return []
        }

        guard let bridge = bridgeClass.perform(NSSelectorFromString("sharedInstance"))?.takeUnretainedValue() as? NSObject else {
            return []
        }

        let selector = NSSelectorFromString("getAvailablePlaylists")
        guard bridge.responds(to: selector),
              let result = bridge.perform(selector)?.takeUnretainedValue() as? [AnyObject] else {
            return []
        }

        return result.compactMap { playlistObj in
            guard let playlist = playlistObj as? NSObject,
                  let id = playlist.value(forKey: "playlistId") as? Int32,
                  let name = playlist.value(forKey: "playlistName") as? String,
                  let size = playlist.value(forKey: "playlistSize") as? Int32 else {
                return nil
            }
            return (id: Int(id), name: name, size: Int(size))
        }
    }

    func playPlaylist(playlistId: Int, startIndex: Int = 0) -> Bool {
//        print("🔧 playPlaylist called with ID: \(playlistId), startIndex: \(startIndex)")

        guard let bridgeClass = NSClassFromString("ModizerPlaylistBridge") as? NSObject.Type,
              let bridge = bridgeClass.perform(NSSelectorFromString("sharedInstance"))?.takeUnretainedValue() as? NSObject else {
            print("❌ Failed to get ModizerPlaylistBridge")
            return false
        }

//        print("✅ Got ModizerPlaylistBridge instance")

        let selector = NSSelectorFromString("playPlaylistWithId:startIndex:")
        guard bridge.responds(to: selector) else {
            print("❌ Bridge doesn't respond to playPlaylistWithId:startIndex:")
            return false
        }

//        print("✅ Bridge responds to selector")

        let method = bridge.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector, Int32, Int32) -> Bool
        let implementation = unsafeBitCast(method, to: MethodType.self)

        let result = implementation(bridge, selector, Int32(playlistId), Int32(startIndex))
//        print("🔧 playPlaylistWithId:startIndex: returned: \(result)")

        return result
    }
    
    func playBuiltinPlaylist(playlistId: Int, startIndex: Int = 0) -> Bool {
        guard let bridgeClass = NSClassFromString("ModizerPlaylistBridge") as? NSObject.Type,
              let bridge = bridgeClass.perform(NSSelectorFromString("sharedInstance"))?.takeUnretainedValue() as? NSObject else {
            return false
        }

        let selector = NSSelectorFromString("playBuiltinPlaylistWithId:startIndex:")
        guard bridge.responds(to: selector) else { return false }

        let method = bridge.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector, Int32, Int32) -> Bool
        let implementation = unsafeBitCast(method, to: MethodType.self)

        return implementation(bridge, selector, Int32(playlistId), Int32(startIndex))
    }

    // MARK: - Fuzzy Matching Helper

    /// Cache pour les mappings nombre→texte par locale
    private static var numberToWordCache: [String: [Int: String]] = [:]

    /// Génère un mapping nombre→texte pour une locale donnée
    private static func generateNumberMapping(for locale: Locale, upTo maxNumber: Int = 100) -> [Int: String] {
        let cacheKey = locale.identifier
        if let cached = numberToWordCache[cacheKey] {
            return cached
        }

        let formatter = NumberFormatter()
        formatter.numberStyle = .spellOut
        formatter.locale = locale

        var mapping: [Int: String] = [:]
        for i in 0...maxNumber {
            if let spelled = formatter.string(from: NSNumber(value: i)) {
                mapping[i] = spelled.lowercased()
            }
        }

        numberToWordCache[cacheKey] = mapping
        return mapping
    }

    /// Normalise les nombres écrits en chiffres (multi-langue avec NumberFormatter)
    private func normalizeNumbers(in text: String) -> String {
        var result = text.lowercased()

        // Récupérer les mappings pour toutes les langues supportées
        let frenchMapping = Self.generateNumberMapping(for: Locale(identifier: "fr_FR"), upTo: 100)
        let englishMapping = Self.generateNumberMapping(for: Locale(identifier: "en_US"), upTo: 100)
        let germanMapping = Self.generateNumberMapping(for: Locale(identifier: "de_DE"), upTo: 100)
        let spanishMapping = Self.generateNumberMapping(for: Locale(identifier: "es_ES"), upTo: 100)
        let italianMapping = Self.generateNumberMapping(for: Locale(identifier: "it_IT"), upTo: 100)
        let swedishMapping = Self.generateNumberMapping(for: Locale(identifier: "sv_SE"), upTo: 100)
        let norwegianMapping = Self.generateNumberMapping(for: Locale(identifier: "nb_NO"), upTo: 100)
        let portugueseMapping = Self.generateNumberMapping(for: Locale(identifier: "pt_BR"), upTo: 100)
        let russianMapping = Self.generateNumberMapping(for: Locale(identifier: "ru_RU"), upTo: 100)
        let arabicMapping = Self.generateNumberMapping(for: Locale(identifier: "ar_SA"), upTo: 100)
        let hindiMapping = Self.generateNumberMapping(for: Locale(identifier: "hi_IN"), upTo: 100)
        let japaneseMapping = Self.generateNumberMapping(for: Locale(identifier: "ja_JP"), upTo: 100)
        let chineseMapping = Self.generateNumberMapping(for: Locale(identifier: "zh_CN"), upTo: 100)
        let danishMapping = Self.generateNumberMapping(for: Locale(identifier: "da_DK"), upTo: 100)
        let dutchMapping = Self.generateNumberMapping(for: Locale(identifier: "nl_NL"), upTo: 100)
        let koreanMapping = Self.generateNumberMapping(for: Locale(identifier: "ko_KR"), upTo: 100)

        // Combiner tous les mappings (inverser: mot→nombre)
        var wordToNumber: [String: String] = [:]
        for (number, word) in frenchMapping {
            wordToNumber[word] = "\(number)"
        }
        for (number, word) in englishMapping {
            wordToNumber[word] = "\(number)"
        }
        for (number, word) in germanMapping {
            wordToNumber[word] = "\(number)"
        }
        for (number, word) in spanishMapping {
            wordToNumber[word] = "\(number)"
        }
        for (number, word) in italianMapping {
            wordToNumber[word] = "\(number)"
        }
        for (number, word) in swedishMapping {
            wordToNumber[word] = "\(number)"
        }
        for (number, word) in norwegianMapping {
            wordToNumber[word] = "\(number)"
        }
        for (number, word) in portugueseMapping {
            wordToNumber[word] = "\(number)"
        }
        for (number, word) in russianMapping {
            wordToNumber[word] = "\(number)"
        }
        for (number, word) in arabicMapping {
            wordToNumber[word] = "\(number)"
        }
        for (number, word) in hindiMapping {
            wordToNumber[word] = "\(number)"
        }
        for (number, word) in japaneseMapping {
            wordToNumber[word] = "\(number)"
        }
        for (number, word) in chineseMapping {
            wordToNumber[word] = "\(number)"
        }
        for (number, word) in danishMapping {
            wordToNumber[word] = "\(number)"
        }
        for (number, word) in dutchMapping {
            wordToNumber[word] = "\(number)"
        }
        for (number, word) in koreanMapping {
            wordToNumber[word] = "\(number)"
        }

        // Trier par longueur décroissante pour matcher les expressions les plus longues en premier
        // (ex: "vingt et un" avant "vingt")
        let sortedWords = wordToNumber.keys.sorted { $0.count > $1.count }

        for word in sortedWords {
            if let digit = wordToNumber[word] {
                // Utiliser des boundary matchers pour éviter de remplacer des parties de mots
                let pattern = "\\b\(NSRegularExpression.escapedPattern(for: word))\\b"
                if let regex = try? NSRegularExpression(pattern: pattern, options: .caseInsensitive) {
                    let range = NSRange(result.startIndex..., in: result)
                    result = regex.stringByReplacingMatches(in: result, range: range, withTemplate: digit)
                }
            }
        }

        return result
    }

    /// Normalisation phonétique : utilise CFStringTransform pour la translittération
    private func phoneticNormalize(_ text: String) -> String {
        var result = text.lowercased()

        // 1. Enlever les mots vides courants (stopwords) qui peuvent être ajoutés par Siri
        let stopwords = [
            // Français (les plus courants qui posent problème)
            "c'est", "cest", "c est",
            // Anglais
            "it's", "its", "it is",
        ]

        for stopword in stopwords.sorted(by: { $0.count > $1.count }) {
            // Au début de la chaîne suivi d'un espace
            result = result.replacingOccurrences(of: "^\(NSRegularExpression.escapedPattern(for: stopword))\\s+", with: "", options: .regularExpression)
        }

        // 2. Convertir en représentation phonétique via CFStringTransform
        // Strip diacritics (accents)
        if let transformed = result.applyingTransform(.stripDiacritics, reverse: false) {
            result = transformed
        }

        // 3. Enlever apostrophes, tirets et caractères spéciaux
        result = result.replacingOccurrences(of: "'", with: "")
        result = result.replacingOccurrences(of: "'", with: "")
        result = result.replacingOccurrences(of: "-", with: "")
        result = result.replacingOccurrences(of: "_", with: "")

        // 4. Normaliser les espaces
        result = result.replacingOccurrences(of: "\\s+", with: "", options: .regularExpression)
        result = result.trimmingCharacters(in: .whitespaces)

        return result
    }

    /// Génère une clé phonétique simplifiée type Soundex
    private func phoneticKey(_ text: String) -> String {
        let normalized = phoneticNormalize(text)
        var key = ""

        // Garder la première lettre
        if let first = normalized.first {
            key.append(first)
        }

        // Simplifier les consonnes en groupes phonétiques
        let phoneticMap: [Character: Character] = [
            // Labiales
            "b": "1", "p": "1", "f": "1", "v": "1",
            // Dentales/Alveolaires
            "c": "2", "s": "2", "k": "2", "g": "2", "j": "2", "q": "2", "x": "2", "z": "2",
            // Liquides
            "l": "3", "r": "3",
            // Nasales
            "m": "4", "n": "4",
            // Dentales
            "d": "5", "t": "5"
        ]

        var prevCode: Character = "0"

        for char in normalized.lowercased().dropFirst() {
            if let code = phoneticMap[char] {
                if code != prevCode {
                    key.append(code)
                    prevCode = code
                }
            } else if "aeiouy".contains(char) {
                // Voyelles - reset le code précédent mais ne pas ajouter
                prevCode = "0"
            }
        }

        return key
    }

    /// Calcule la distance de Levenshtein entre deux strings (nombre de changements nécessaires)
    private func levenshteinDistance(_ s1: String, _ s2: String) -> Int {
        let s1 = Array(s1)
        let s2 = Array(s2)
        let m = s1.count
        let n = s2.count

        var matrix = Array(repeating: Array(repeating: 0, count: n + 1), count: m + 1)

        for i in 0...m { matrix[i][0] = i }
        for j in 0...n { matrix[0][j] = j }

        for i in 1...m {
            for j in 1...n {
                let cost = s1[i-1] == s2[j-1] ? 0 : 1
                matrix[i][j] = min(
                    matrix[i-1][j] + 1,      // deletion
                    matrix[i][j-1] + 1,      // insertion
                    matrix[i-1][j-1] + cost  // substitution
                )
            }
        }

        return matrix[m][n]
    }

    /// Trouve la playlist la plus proche du nom donné avec tolérance (casse, accents, nombres, phonétique)
    func findBestMatchingPlaylist(for searchName: String) -> (id: Int, name: String, size: Int)? {
        let playlists = getAvailablePlaylists()

        guard !playlists.isEmpty else { return nil }

        // Log pour debug - voir ce que Siri envoie exactement
//        print("🎯 Siri search input: '\(searchName)'")

        // Normaliser la recherche (nombres → chiffres, puis minuscules, sans accents)
        let withNumbers = normalizeNumbers(in: searchName)
//        print("🎯 After normalizeNumbers: '\(withNumbers)'")

        let normalizedSearch = withNumbers.folding(options: [.diacriticInsensitive, .caseInsensitive], locale: .current)
//        print("🎯 After folding: '\(normalizedSearch)'")

        let normalizedSearchNoSpaces = normalizedSearch.replacingOccurrences(of: " ", with: "")
//        print("🎯 Without spaces: '\(normalizedSearchNoSpaces)'")

        // Version phonétique (enlever les mots vides et ne garder que les sons importants)
        let phoneticSearch = phoneticNormalize(normalizedSearch)
//        print("🎯 Phonetic version: '\(phoneticSearch)'")

        // Clé phonétique type Soundex
        let phoneticSearchKey = phoneticKey(normalizedSearch)
//        print("🎯 Phonetic key: '\(phoneticSearchKey)'")

        // 1. Essai exact match (insensible à la casse, accents et nombres)
        if let exactMatch = playlists.first(where: {
            let playlistNormalized = normalizeNumbers(in: $0.name)
                .folding(options: [.diacriticInsensitive, .caseInsensitive], locale: .current)
//            print("  🔍 Exact: '\(normalizedSearch)' vs '\(playlistNormalized)' (playlist: \($0.name))")
            return playlistNormalized == normalizedSearch
        }) {
//            print("  ✅ Exact match found: \(exactMatch.name)")
            return exactMatch
        }

        // 1b. Essai exact match sans espaces (pour gérer "c 64" vs "c64")
        if let exactMatchNoSpaces = playlists.first(where: {
            let playlistNormalized = normalizeNumbers(in: $0.name)
                .folding(options: [.diacriticInsensitive, .caseInsensitive], locale: .current)
                .replacingOccurrences(of: " ", with: "")
//            print("  🔍 Exact no-space: '\(normalizedSearchNoSpaces)' vs '\(playlistNormalized)' (playlist: \($0.name))")
            return playlistNormalized == normalizedSearchNoSpaces
        }) {
//            print("  ✅ Exact no-space match found: \(exactMatchNoSpaces.name)")
            return exactMatchNoSpaces
        }

        // 2. Essai contains (la recherche est contenue dans le nom)
        if let containsMatch = playlists.first(where: {
            let playlistNormalized = normalizeNumbers(in: $0.name)
                .folding(options: [.diacriticInsensitive, .caseInsensitive], locale: .current)
            return playlistNormalized.contains(normalizedSearch)
        }) {
            return containsMatch
        }

        // 3. Essai inverse (le nom est contenu dans la recherche)
        if let reverseMatch = playlists.first(where: {
            let playlistNormalized = normalizeNumbers(in: $0.name)
                .folding(options: [.diacriticInsensitive, .caseInsensitive], locale: .current)
            return normalizedSearch.contains(playlistNormalized)
        }) {
            return reverseMatch
        }

        // 3b. Essai avec normalisation phonétique (enlever les mots vides comme "c'est")
        if let phoneticMatch = playlists.first(where: {
            let playlistNormalized = normalizeNumbers(in: $0.name)
                .folding(options: [.diacriticInsensitive, .caseInsensitive], locale: .current)
            let playlistPhonetic = phoneticNormalize(playlistNormalized)
//            print("  🔍 Phonetic: '\(phoneticSearch)' vs '\(playlistPhonetic)' (playlist: \($0.name))")
            return playlistPhonetic == phoneticSearch || playlistPhonetic.contains(phoneticSearch) || phoneticSearch.contains(playlistPhonetic)
        }) {
//            print("  ✅ Phonetic match found: \(phoneticMatch.name)")
            return phoneticMatch
        }

        // 3c. Essai avec clé phonétique Soundex-like (sons similaires)
        if let soundexMatch = playlists.first(where: {
            let playlistNormalized = normalizeNumbers(in: $0.name)
                .folding(options: [.diacriticInsensitive, .caseInsensitive], locale: .current)
            let playlistKey = phoneticKey(playlistNormalized)
//            print("  🔑 Soundex: '\(phoneticSearchKey)' vs '\(playlistKey)' (playlist: \($0.name))")
            // Match si les clés sont identiques ou très proches
            return playlistKey == phoneticSearchKey ||
                   playlistKey.hasPrefix(phoneticSearchKey) ||
                   phoneticSearchKey.hasPrefix(playlistKey)
        }) {
//            print("  ✅ Soundex match found: \(soundexMatch.name)")
            return soundexMatch
        }

        // 4. Matching phonétique avec distance de Levenshtein
        // Calculer la distance pour chaque playlist et trouver la plus proche
        let withDistances = playlists.map { playlist -> (playlist: (id: Int, name: String, size: Int), distance: Int) in
            let normalizedName = normalizeNumbers(in: playlist.name)
                .folding(options: [.diacriticInsensitive, .caseInsensitive], locale: .current)
            let distance = levenshteinDistance(normalizedSearch, normalizedName)
            return (playlist, distance)
        }

        // Trier par distance (la plus petite en premier)
        let sorted = withDistances.sorted { $0.distance < $1.distance }

        // Accepter seulement si la distance est raisonnable (max 3 caractères de différence pour les noms courts, proportionnel pour les longs)
        if let closest = sorted.first {
            let maxAllowedDistance = max(3, closest.playlist.name.count / 3)
            if closest.distance <= maxAllowedDistance {
                return closest.playlist
            }
        }

        return nil
    }

    func playPlaylist(playlistName: String, startIndex: Int = 0) -> Bool {
        // Utiliser le matching intelligent pour trouver la meilleure correspondance
        guard let matchedPlaylist = findBestMatchingPlaylist(for: playlistName) else {
            return false
        }

        // Utiliser l'ID de la playlist trouvée pour garantir le bon lancement
        return playPlaylist(playlistId: matchedPlaylist.id, startIndex: startIndex)
    }
}

@available(iOS 16.0, *)
struct PlaylistEntity: AppEntity, Identifiable {
    static var typeDisplayRepresentation: TypeDisplayRepresentation { TypeDisplayRepresentation(name: LocalizedStringResource("Playlist")) }

    @MainActor
    struct Query: EntityQuery {
        func entities(for identifiers: [PlaylistEntity.ID]) async throws -> [PlaylistEntity] {
            // Query methods can't reliably launch the app, so just return what's available
            let playlists = ModizerPlayerBridge.shared.getAvailablePlaylists()

            // If no playlists available and app might not be running, the Intent will handle this
            if playlists.isEmpty {
                return []
            }

            let idSet = Set(identifiers)
            return playlists
                .filter { idSet.contains(String($0.id)) }
                .map { PlaylistEntity(id: String($0.id), name: $0.name, size: $0.size) }
        }

        func suggestedEntities() async throws -> [PlaylistEntity] {
            let playlists = ModizerPlayerBridge.shared.getAvailablePlaylists()
            return playlists.map { PlaylistEntity(id: String($0.id), name: $0.name, size: $0.size) }
        }

        func entities(matching string: String) async throws -> [PlaylistEntity] {
            let playlists = ModizerPlayerBridge.shared.getAvailablePlaylists()
            let needle = string.trimmingCharacters(in: .whitespacesAndNewlines).lowercased()

            // Si la recherche est vide ou très courte, retourner toutes les playlists
            if needle.isEmpty || needle.count < 2 {
                return playlists.map { PlaylistEntity(id: String($0.id), name: $0.name, size: $0.size) }
            }

            // Recherche flexible : nom contient la recherche
            let matches = playlists.filter {
                $0.name.lowercased().contains(needle) ||
                $0.name.range(of: needle, options: [.caseInsensitive, .diacriticInsensitive]) != nil
            }

            // Si aucun résultat, retourner toutes les playlists pour permettre à l'utilisateur de choisir
            if matches.isEmpty {
                return playlists.map { PlaylistEntity(id: String($0.id), name: $0.name, size: $0.size) }
            }

            return matches.map { PlaylistEntity(id: String($0.id), name: $0.name, size: $0.size) }
        }
    }

    static var defaultQuery: Query { Query() }

    let id: String
    let name: String
    let size: Int

    var displayRepresentation: DisplayRepresentation {
        // Ne pas inclure le nombre de morceaux pour simplifier la reconnaissance vocale Siri
        DisplayRepresentation(
            title: "\(name)"
        )
    }
}

// MARK: - Play Intent
@available(iOS 16.0, *)
struct PlayIntent: AppIntent {
    static let title: LocalizedStringResource = "Play"
    static let description = IntentDescription(LocalizedStringResource("Starts playing music in Modizer."))
    static let openAppWhenRun: Bool = false

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.play()
        return .result()
    }
}

// MARK: - Pause Intent
@available(iOS 16.0, *)
struct PauseIntent: AppIntent {
    static let title: LocalizedStringResource = "Pause"
    static let description = IntentDescription(LocalizedStringResource("Pauses music playback in Modizer."))
    static let openAppWhenRun: Bool = false

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.pause()
        return .result()
    }
}

// MARK: - Stop Intent
@available(iOS 16.0, *)
struct StopIntent: AppIntent {
    static let title: LocalizedStringResource = "Stop"
    static let description = IntentDescription(LocalizedStringResource("Stops music playback in Modizer."))
    static let openAppWhenRun: Bool = false

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.stop()
        return .result()
    }
}

// MARK: - Next Song Intent
@available(iOS 16.0, *)
struct NextSongIntent: AppIntent {
    static let title: LocalizedStringResource = "Next Song"
    static let description = IntentDescription(LocalizedStringResource("Skips to the next song in Modizer."))
    static let openAppWhenRun: Bool = false

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.playNextSong()
        return .result()
    }
}

// MARK: - Previous Song Intent
@available(iOS 16.0, *)
struct PreviousSongIntent: AppIntent {
    static let title: LocalizedStringResource = "Previous Song"
    static let description = IntentDescription(LocalizedStringResource("Skips to the previous song in Modizer."))
    static let openAppWhenRun: Bool = false

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.playPreviousSong()
        return .result()
    }
}

// MARK: - Play Subsong Intent
@available(iOS 16.0, *)
struct PlaySubsongIntent: AppIntent {
    static let title: LocalizedStringResource = "Play Subsong"
    static let description = IntentDescription(LocalizedStringResource("Plays a specific subsong in the current file."))
    static let openAppWhenRun: Bool = false

    @Parameter(title: LocalizedStringResource("Subsong Index"), description: LocalizedStringResource("The index of the subsong to play"), default: 0)
    var subsongIndex: Int

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.playSubsong(index: subsongIndex)
        return .result()
    }
}

// MARK: - Seek Intent
@available(iOS 16.0, *)
struct SeekIntent: AppIntent {
    static let title: LocalizedStringResource = "Seek"
    static let description = IntentDescription(LocalizedStringResource("Seeks to a specific position in the current song."))
    static let openAppWhenRun: Bool = false

    @Parameter(title: LocalizedStringResource("Position (seconds)"), description: LocalizedStringResource("The position to seek to in seconds"))
    var position: Int

    @Parameter(title: LocalizedStringResource("Relative"), description: LocalizedStringResource("Whether to seek relative to current position"), default: false)
    var relative: Bool

    @MainActor
    func perform() async throws -> some IntentResult {
        let targetPosition: Int

        if relative {
            let currentTime = ModizerPlayerBridge.shared.getCurrentTime()
            targetPosition = currentTime + position
        } else {
            targetPosition = position
        }

        ModizerPlayerBridge.shared.seek(to: Int64(targetPosition) * 1000) // Convert to milliseconds
        return .result()
    }
}

// MARK: - Set Infinite Loop Intent
@available(iOS 16.0, *)
struct SetInfiniteLoopIntent: AppIntent {
    static let title: LocalizedStringResource = "Set Infinite Loop"
    static let description = IntentDescription(LocalizedStringResource("Set the infinite loop mode in Modizer."))
    static let openAppWhenRun: Bool = false

    @Parameter(title: LocalizedStringResource("Infinite Loop Mode"), description: LocalizedStringResource("0: off, 1: infinite loop"), default: 1)
    var mode: Int

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.setInfiniteLoopMode(mode)
        return .result()
    }
}

// MARK: - Set Repeat Mode Intent
@available(iOS 16.0, *)
struct SetLoopModeIntent: AppIntent {
    static let title: LocalizedStringResource = "Set Loop Mode"
    static let description = IntentDescription(LocalizedStringResource("Set the loop mode in Modizer."))
    static let openAppWhenRun: Bool = false

    @Parameter(title: LocalizedStringResource("Repeat Mode"), description: LocalizedStringResource("0: off, 1: loop current title, 2: loop playlist"), default: 1)
    var mode: Int

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.setLoopMode(mode)
        return .result()
    }
}

// MARK: - Set Shuffle Mode Intent
@available(iOS 16.0, *)
struct SetShuffleModeIntent: AppIntent {
    static let title: LocalizedStringResource = "Set Shuffle Mode"
    static let description = IntentDescription(LocalizedStringResource("Set the shuffle mode in Modizer."))
    static let openAppWhenRun: Bool = false

    @Parameter(title: LocalizedStringResource("Shuffle Mode"), description: LocalizedStringResource("0: off, 1: choose only one title if file has subsongs or is an archive, 2: play all titles of a given file"), default: 1)
    var mode: Int

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.setShuffleMode(mode)
        return .result()
    }
}

// MARK: - Toggle Shuffle Intent
@available(iOS 16.0, *)
struct ToggleShuffleIntent: AppIntent {
    static let title: LocalizedStringResource = "Toggle Shuffle"
    static let description = IntentDescription(LocalizedStringResource("Toggles shuffle mode in Modizer."))
    static let openAppWhenRun: Bool = false

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.toggleShuffle()
        return .result()
    }
}

// MARK: - Toggle Loop Mode Intent
@available(iOS 16.0, *)
struct ToggleLoopModeIntent: AppIntent {
    static let title: LocalizedStringResource = "Toggle Loop Mode"
    static let description = IntentDescription(LocalizedStringResource("Toggles loop mode in Modizer."))
    static let openAppWhenRun: Bool = false

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.toggleLoopMode()
        return .result()
    }
}

// MARK: - Set Fullscreen Mode
@available(iOS 16.0, *)
struct SetFullscreenIntent: AppIntent {
    static let title: LocalizedStringResource = "Set Fullscreen mode"
    static let description = IntentDescription(LocalizedStringResource("Set Fullscreen mode on or off in Modizer."))
    static let openAppWhenRun: Bool = true

    @Parameter(title: LocalizedStringResource("Fullscreen mode"), description: LocalizedStringResource("Activate (1) or disactivate (0) Fullscreen mode"), default: 1)
    var fxValue: Int

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.setFX(0, value:fxValue)
        return .result()
    }
}

// MARK: - Go the player view
@available(iOS 16.0, *)
struct GoToPlayerViewIntent: AppIntent {
    static let title: LocalizedStringResource = "Go the player view"
    static let description = IntentDescription(LocalizedStringResource("Go the player view in Modizer."))
    static let openAppWhenRun: Bool = true
    
    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.goToPlayerView()
        return .result()
    }
}

// MARK: - Activate Milkdrop FX
@available(iOS 16.0, *)
struct SetFXIntent: AppIntent {
    static let title: LocalizedStringResource = "Set FX"
    static let description = IntentDescription(LocalizedStringResource("Set FX mode in Modizer."))
    static let openAppWhenRun: Bool = true

    @Parameter(
        title: LocalizedStringResource("FX"),
        description: LocalizedStringResource("The FX to set"),
        optionsProvider: FXOptionsProvider()
    )
    var fxName: String

    @Parameter(title: LocalizedStringResource("FX Mode"), description: LocalizedStringResource("The mode to activate for the selected FX (0:off)"), default: 0)
    var fxValue: Int

    @MainActor
    func perform() async throws -> some IntentResult {
        
        let fxList = FXOptionsProvider.fxList
                guard let fxIndex = fxList.firstIndex(of: fxName) else {
                    return .result()
                }
        
        ModizerPlayerBridge.shared.setFX(fxIndex+1, value:fxValue)
        return .result()
    }
}

// Le provider qui fournit la liste
struct FXOptionsProvider: DynamicOptionsProvider {
    // Liste statique accessible
        static let fxList = [
            NSLocalizedString("ProjectM", comment: ""),
            NSLocalizedString("Oscillo", comment: ""),
            NSLocalizedString("Piano roll", comment: ""),
            NSLocalizedString("3D Piano", comment: ""),
            NSLocalizedString("Notes bars", comment: ""),
            NSLocalizedString("Tracker", comment: ""),
            NSLocalizedString("2D Spectrum", comment: ""),
            NSLocalizedString("3D Spectrum", comment: ""),
            NSLocalizedString("3D Landscape", comment: "")
        ]
    func results() async throws -> [String] {
            return FXOptionsProvider.fxList
        }
}


// MARK: - Get Now Playing Intent
@available(iOS 16.0, *)
struct GetNowPlayingIntent: AppIntent {
    static let title: LocalizedStringResource = "Get Now Playing"
    static let description = IntentDescription(LocalizedStringResource("Gets information about the currently playing song."))
    static let openAppWhenRun: Bool = false

    @MainActor
    func perform() async throws -> some IntentResult & ReturnsValue<String> & ProvidesDialog {
        let title = ModizerPlayerBridge.shared.getCurrentSongTitle()
        let album = ModizerPlayerBridge.shared.getCurrentSongAlbum()
        let artist = ModizerPlayerBridge.shared.getCurrentSongArtist()
        let isPlaying = ModizerPlayerBridge.shared.isPlaying()
        let currentTime = ModizerPlayerBridge.shared.getCurrentTime()
        let totalTime = ModizerPlayerBridge.shared.getSongLength()

        let status = isPlaying ? "Playing" : "Paused"
        let info = """
        \(status): \(title) from \(album)
        Time: \(currentTime)s / \(totalTime)s
        """
        // Localized speakable summary with fallbacks
        let unknownTitle = NSLocalizedString("unknown_title", tableName: nil, bundle: .main, value: "Titre inconnu", comment: "Fallback when the current song title is unknown")
        let unknownAlbum = NSLocalizedString("unknown_album", tableName: nil, bundle: .main, value: "Album inconnu", comment: "Fallback when the current album is unknown")
        let safeTitle = title.isEmpty ? unknownTitle : title
        let safeAlbum = album.isEmpty ? unknownAlbum : album
        let format = NSLocalizedString("now_playing_format", tableName: nil, bundle: .main, value: "Playing: %@ from %@", comment: "Spoken summary: Now playing <title> from <album>")
        let speakableInfo = String.localizedStringWithFormat(format, safeTitle, safeAlbum)
        
        // Demander explicitement à Siri d’énoncer le résultat
        return .result(
            value: info,
            dialog: IntentDialog(stringLiteral: speakableInfo)
        )

        //return .result(value: info)
    }
}

// MARK: - Get Playlists Intent
@available(iOS 16.0, *)
struct GetPlaylistsIntent: AppIntent {
    static let title: LocalizedStringResource = "Get Playlists"
    static let description = IntentDescription(LocalizedStringResource("Gets the list of available playlists in Modizer."))
    static let openAppWhenRun: Bool = true

    @MainActor
    func perform() async throws -> some IntentResult & ReturnsValue<String> {
        // Wait for app to initialize if needed (max 5 seconds)
        var attempts = 0
        var playlists = ModizerPlayerBridge.shared.getAvailablePlaylists()
        while playlists.isEmpty && attempts < 50 {
            try? await Task.sleep(nanoseconds: 100_000_000) // 100ms
            attempts += 1
            playlists = ModizerPlayerBridge.shared.getAvailablePlaylists()
        }

        if playlists.isEmpty {
            return .result(value: NSLocalizedString("No playlists found", comment: ""))
        }

        let playlistList = playlists.map { playlist in
            "\(playlist.name) (ID: \(playlist.id), \(playlist.size) songs)"
        }.joined(separator: "\n")

        return .result(value: "Available playlists:\n\(playlistList)")
    }
}

// MARK: - Play random picks Playlist Intent
@available(iOS 16.0, *)
struct PlayRandomPicksIntent: AppIntent {
    static let title: LocalizedStringResource = "Play some music"
    static let description = IntentDescription(LocalizedStringResource("Random picks from available local titles in Modizer."))
    static let openAppWhenRun: Bool = true

    @MainActor
    func perform() async throws -> some IntentResult {
        // Set the flag to skip animation and auto-restart
        UserDefaults.standard.set(true, forKey: "LaunchedFromShortcut")
        UserDefaults.standard.synchronize()

        // Call ModizerPlaylistBridge via runtime for random picks (id = -1)
        _ = ModizerPlayerBridge.shared.playBuiltinPlaylist(playlistId: -1, startIndex: 0)
        return .result()
    }
}

// MARK: - Play most played Playlist Intent
@available(iOS 16.0, *)
struct PlayMostPlayedIntent: AppIntent {
    static let title: LocalizedStringResource = "Play most played"
    static let description = IntentDescription(LocalizedStringResource("Most played titles from available local titles in Modizer."))
    static let openAppWhenRun: Bool = true

    @MainActor
    func perform() async throws -> some IntentResult {
        // Set the flag to skip animation and auto-restart
        UserDefaults.standard.set(true, forKey: "LaunchedFromShortcut")
        UserDefaults.standard.synchronize()

        // Call ModizerPlaylistBridge via runtime for most played (id = -2)
        _ = ModizerPlayerBridge.shared.playBuiltinPlaylist(playlistId: -2, startIndex: 0)
        return .result()
    }
}

// MARK: - Play favorites Playlist Intent
@available(iOS 16.0, *)
struct PlayFavoritesIntent: AppIntent {
    static let title: LocalizedStringResource = "Play favorites"
    static let description = IntentDescription(LocalizedStringResource("Favorites titles from available local titles in Modizer."))
    static let openAppWhenRun: Bool = true

    @MainActor
    func perform() async throws -> some IntentResult {
        // Set the flag to skip animation and auto-restart
        UserDefaults.standard.set(true, forKey: "LaunchedFromShortcut")
        UserDefaults.standard.synchronize()

        // Call ModizerPlaylistBridge via runtime for favorites (id = -3)
        _ = ModizerPlayerBridge.shared.playBuiltinPlaylist(playlistId: -3, startIndex: 0)
        return .result()
    }
}

// MARK: - Play Playlist Intent (with entity - for Shortcuts app)
@available(iOS 16.0, *)
struct PlayPlaylistIntent: AppIntent {
    static let title: LocalizedStringResource = "Play Playlist"
    static let description = IntentDescription(LocalizedStringResource("Plays a specific playlist in Modizer. Note: For best results with voice commands, use 'Play Playlist By Name' instead."))
    static let openAppWhenRun: Bool = true

    // This intent should appear after the by-name version in voice command suggestions
    static let isDiscoverable = true

    static var parameterSummary: some ParameterSummary {
        Summary("Play \(\.$playlist)") {
            \.$startIndex
        }
    }

    @Parameter(
        title: LocalizedStringResource("Playlist"),
        description: LocalizedStringResource("The playlist to play")
    )
    var playlist: PlaylistEntity

    @Parameter(title: LocalizedStringResource("Start Index"), description: LocalizedStringResource("The index of the song to start playing from (0-based)"), default: 0)
    var startIndex: Int

    @MainActor
    func perform() async throws -> some IntentResult & ProvidesDialog {
        // Set the flag to skip animation and auto-restart
        UserDefaults.standard.set(true, forKey: "LaunchedFromShortcut")
        UserDefaults.standard.synchronize()

        // Call ModizerPlaylistBridge via runtime
        var success = false
        if let bridgeClass = NSClassFromString("ModizerPlaylistBridge") as? NSObject.Type,
           let bridge = bridgeClass.perform(NSSelectorFromString("sharedInstance"))?.takeUnretainedValue() as? NSObject,
           let playlistIdInt = Int32(playlist.id) {
            let selector = NSSelectorFromString("playPlaylistWithId:startIndex:")
            if bridge.responds(to: selector) {
                let sig = bridge.method(for: selector)
                typealias Function = @convention(c) (AnyObject, Selector, Int32, Int32) -> Bool
                let function = unsafeBitCast(sig, to: Function.self)
                success = function(bridge, selector, playlistIdInt, Int32(startIndex))
            }
        }

        if !success {
            throw PlaylistError.playlistNotFound
        }

        let fmt = NSLocalizedString("play_playlist_dialog", tableName: nil, bundle: .main, value: "Lecture de la playlist %@", comment: "Dialog confirming playlist playback")
        let dialog = String(format: fmt, playlist.name)
        return .result(dialog: IntentDialog(stringLiteral: dialog))
    }
}

// MARK: - Play Playlist By Name Intent (with string - for voice commands)
@available(iOS 16.0, *)
struct PlayPlaylistByNameIntent: AppIntent {
    static let title: LocalizedStringResource = "Play Playlist By Name"
    static let description = IntentDescription(LocalizedStringResource("Plays a playlist by name in Modizer. Recommended for voice commands."))
    static let openAppWhenRun: Bool = true

    // Make this the preferred intent for voice commands
    static let isDiscoverable = true

    static var parameterSummary: some ParameterSummary {
        Summary("Play playlist \(\.$playlistName)")
    }

    @Parameter(
        title: LocalizedStringResource("Playlist Name"),
        description: LocalizedStringResource("The name of the playlist to play"),
        requestValueDialog: IntentDialog(LocalizedStringResource("Which playlist would you like to play?"))
    )
    var playlistName: String

    @Parameter(title: LocalizedStringResource("Start Index"), description: LocalizedStringResource("The index of the song to start playing from (0-based)"), default: 0)
    var startIndex: Int

    @MainActor
    func perform() async throws -> some IntentResult & ProvidesDialog {
//        print("🎵 PlayPlaylistByNameIntent.perform() called with: '\(playlistName)'")

        // Set the flag to skip animation and auto-restart
        UserDefaults.standard.set(true, forKey: "LaunchedFromShortcut")
        UserDefaults.standard.synchronize()

        // Find the best matching playlist using intelligent matching
        guard let matchedPlaylist = ModizerPlayerBridge.shared.findBestMatchingPlaylist(for: playlistName) else {
            print("❌ No matching playlist found")
            throw PlaylistError.playlistNotFound
        }

//        print("🎵 Matched playlist: '\(matchedPlaylist.name)' (ID: \(matchedPlaylist.id), size: \(matchedPlaylist.size))")

        // Check if playlist has entries
        if matchedPlaylist.size == 0 {
            print("❌ Playlist is empty")
            throw PlaylistError.playlistEmpty
        }

        // Play the matched playlist
        let success = ModizerPlayerBridge.shared.playPlaylist(playlistId: matchedPlaylist.id, startIndex: startIndex)

//        print("🎵 playPlaylist returned: \(success)")

        if !success {
            print("❌ Failed to play playlist")
            throw PlaylistError.playlistNotFound
        }

        // Use the actual matched playlist name in the dialog (not what Siri heard)
        let fmt = NSLocalizedString("play_playlist_dialog", tableName: nil, bundle: .main, value: "Lecture de la playlist %@", comment: "Dialog confirming playlist playback")
        let dialog = String(format: fmt, matchedPlaylist.name)
//        print("✅ Returning success dialog: '\(dialog)'")
        return .result(dialog: IntentDialog(stringLiteral: dialog))
    }
}

@available(iOS 16.0, *)
enum PlaylistError: Error, CustomLocalizedStringResourceConvertible {
    case playlistNotFound
    case playlistEmpty

    var localizedStringResource: LocalizedStringResource {
        switch self {
        case .playlistNotFound:
            return LocalizedStringResource("playlist_not_found_error", defaultValue: "Playlist not found. Use 'Get Playlists' to see available playlists.")
        case .playlistEmpty:
            return LocalizedStringResource("playlist_empty_error", defaultValue: "This playlist has no entries. Please add some music to it first.")
        }
    }
}

// MARK: - App Shortcuts Provider
@available(iOS 16.0, *)
struct ModizerShortcuts: AppShortcutsProvider {
    static var appShortcuts: [AppShortcut] {
        return [
            AppShortcut(
                intent: PlayPlaylistByNameIntent(),
                phrases: [
                    // English
                    "Play a playlist in \(.applicationName)",
                    "Start a playlist in \(.applicationName)",
                    "Play playlist in \(.applicationName)",
                    // French
                    "Joue une playlist dans \(.applicationName)",
                    "Lance une playlist dans \(.applicationName)",
                    // German
                    "Spiele eine Playlist in \(.applicationName)",
                    "Starte eine Playlist in \(.applicationName)",
                    // Spanish
                    "Reproduce una lista en \(.applicationName)",
                    "Inicia una lista en \(.applicationName)",
                    // Italian
                    "Riproduci una playlist in \(.applicationName)",
                    "Avvia una playlist in \(.applicationName)",
                    // Swedish
                    "Spela en spellista i \(.applicationName)",
                    "Starta en spellista i \(.applicationName)",
                    // Norwegian
                    "Spill en spilleliste i \(.applicationName)",
                    "Start en spilleliste i \(.applicationName)",
                    // Portuguese
                    "Reproduzir uma lista em \(.applicationName)",
                    "Iniciar uma lista em \(.applicationName)",
                    // Russian
                    "Воспроизвести плейлист в \(.applicationName)",
                    "Запустить плейлист в \(.applicationName)",
                    // Arabic
                    "تشغيل قائمة تشغيل في \(.applicationName)",
                    "ابدأ قائمة تشغيل في \(.applicationName)",
                    // Hindi
                    "\(.applicationName) में प्लेलिस्ट चलाएं",
                    "\(.applicationName) में एक प्लेलिस्ट शुरू करें",
                    // Japanese
                    "\(.applicationName)でプレイリストを再生",
                    "\(.applicationName)のプレイリスト",
                    // Chinese
                    "在\(.applicationName)中播放播放列表",
                    "播放\(.applicationName)的播放列表",
                    // Danish
                    "Afspil en playliste i \(.applicationName)",
                    "Start en playliste i \(.applicationName)",
                    // Dutch
                    "Speel een afspeellijst in \(.applicationName)",
                    "Start een afspeellijst in \(.applicationName)",
                    // Korean
                    "\(.applicationName)에서 재생목록 재생",
                    "\(.applicationName)에서 플레이리스트 시작",
                    // Czech
                    "Přehrát playlist v \(.applicationName)",
                    "Spustit playlist v \(.applicationName)",
                    // Polish
                    "Odtwórz playlistę w \(.applicationName)",
                    "Uruchom playlistę w \(.applicationName)",
                    // Finnish
                    "Toista soittolista \(.applicationName)ssa",
                    "Käynnistä soittolista \(.applicationName)ssa"
                ],
                shortTitle: "Play Playlist",
                systemImageName: "music.note.list"
            ),
            AppShortcut(
                intent: PlayRandomPicksIntent(),
                phrases: [
                    // English
                    "Play some music in \(.applicationName)",
                    "Start random playlist in \(.applicationName)",
                    // French
                    "Joue de la musique dans \(.applicationName)",
                    "Lance la playlist aléatoire dans \(.applicationName)",
                    // German
                    "Spiele Musik in \(.applicationName)",
                    "Starte zufällige Wiedergabe in \(.applicationName)",
                    // Spanish
                    "Reproduce música en \(.applicationName)",
                    "Inicia lista aleatoria en \(.applicationName)",
                    // Italian
                    "Riproduci musica in \(.applicationName)",
                    "Avvia playlist casuale in \(.applicationName)",
                    // Swedish
                    "Spela musik i \(.applicationName)",
                    "Starta slumpmässig spellista i \(.applicationName)",
                    // Norwegian
                    "Spill musikk i \(.applicationName)",
                    "Start tilfeldig spilleliste i \(.applicationName)",
                    // Portuguese
                    "Reproduzir música em \(.applicationName)",
                    "Iniciar lista aleatória em \(.applicationName)",
                    // Russian
                    "Воспроизвести музыку в \(.applicationName)",
                    "Запустить случайный плейлист в \(.applicationName)",
                    // Arabic
                    "تشغيل موسيقى في \(.applicationName)",
                    "ابدأ قائمة عشوائية في \(.applicationName)",
                    // Hindi
                    "\(.applicationName) में संगीत चलाएं",
                    "\(.applicationName) में रैंडम प्लेलिस्ट शुरू करें",
                    // Japanese
                    "\(.applicationName)で音楽を再生",
                    "\(.applicationName)でランダム再生",
                    // Chinese
                    "在\(.applicationName)中播放音乐",
                    "在\(.applicationName)中随机播放",
                    // Danish
                    "Afspil musik i \(.applicationName)",
                    "Start tilfældig playliste i \(.applicationName)",
                    // Dutch
                    "Speel muziek in \(.applicationName)",
                    "Start willekeurige afspeellijst in \(.applicationName)",
                    // Korean
                    "\(.applicationName)에서 음악 재생",
                    "\(.applicationName)에서 무작위 재생",
                    // Czech
                    "Přehrát hudbu v \(.applicationName)",
                    "Spustit náhodný playlist v \(.applicationName)",
                    // Polish
                    "Odtwórz muzykę w \(.applicationName)",
                    "Uruchom losową playlistę w \(.applicationName)",
                    // Finnish
                    "Toista musiikkia \(.applicationName)ssa",
                    "Käynnistä satunnainen soittolista \(.applicationName)ssa"
                ],
                shortTitle: "Play some music",
                systemImageName: "play.fill"
            ),
            AppShortcut(
                intent: PlayFavoritesIntent(),
                phrases: [
                    // English
                    "Play favorites in \(.applicationName)",
                    "Start favorites playlist in \(.applicationName)",
                    // French
                    "Joue les favoris dans \(.applicationName)",
                    "Lance la playlist favoris dans \(.applicationName)",
                    // German
                    "Spiele Favoriten in \(.applicationName)",
                    "Starte Favoriten-Playlist in \(.applicationName)",
                    // Spanish
                    "Reproduce favoritos en \(.applicationName)",
                    "Inicia lista de favoritos en \(.applicationName)",
                    // Italian
                    "Riproduci preferiti in \(.applicationName)",
                    "Avvia playlist preferiti in \(.applicationName)",
                    // Swedish
                    "Spela favoriter i \(.applicationName)",
                    "Starta favoritspellista i \(.applicationName)",
                    // Norwegian
                    "Spill favoritter i \(.applicationName)",
                    "Start favorittspilleliste i \(.applicationName)",
                    // Portuguese
                    "Reproduzir favoritos em \(.applicationName)",
                    "Iniciar lista de favoritos em \(.applicationName)",
                    // Russian
                    "Воспроизвести избранное в \(.applicationName)",
                    "Запустить плейлист избранного в \(.applicationName)",
                    // Arabic
                    "تشغيل المفضلة في \(.applicationName)",
                    "ابدأ قائمة المفضلة في \(.applicationName)",
                    // Hindi
                    "\(.applicationName) में पसंदीदा चलाएं",
                    "\(.applicationName) में पसंदीदा प्लेलिस्ट शुरू करें",
                    // Japanese
                    "\(.applicationName)でお気に入りを再生",
                    "\(.applicationName)のお気に入りを再生",
                    // Chinese
                    "在\(.applicationName)中播放收藏",
                    "播放\(.applicationName)的收藏列表",
                    // Danish
                    "Afspil favoritter i \(.applicationName)",
                    "Start favorit playliste i \(.applicationName)",
                    // Dutch
                    "Speel favorieten in \(.applicationName)",
                    "Start favorieten afspeellijst in \(.applicationName)",
                    // Korean
                    "\(.applicationName)에서 즐겨찾기 재생",
                    "\(.applicationName)에서 좋아하는 음악 재생",
                    // Czech
                    "Přehrát oblíbené v \(.applicationName)",
                    "Spustit playlist oblíbených v \(.applicationName)",
                    // Polish
                    "Odtwórz ulubione w \(.applicationName)",
                    "Uruchom playlistę ulubionych w \(.applicationName)",
                    // Finnish
                    "Toista suosikit \(.applicationName)ssa",
                    "Käynnistä suosikkisoittolista \(.applicationName)ssa"
                ],
                shortTitle: "Play favorites music",
                systemImageName: "play.fill"
            ),
            AppShortcut(
                intent: PlayMostPlayedIntent(),
                phrases: [
                    // English
                    "Play most played in \(.applicationName)",
                    "Start most played playlist in \(.applicationName)",
                    // French
                    "Joue les plus joués dans \(.applicationName)",
                    "Lance la playlist les plus joués dans \(.applicationName)",
                    // German
                    "Spiele meistgespielte in \(.applicationName)",
                    "Starte meistgespielte Playlist in \(.applicationName)",
                    // Spanish
                    "Reproduce más reproducidos en \(.applicationName)",
                    "Inicia lista más reproducidos en \(.applicationName)",
                    // Italian
                    "Riproduci più riprodotti in \(.applicationName)",
                    "Avvia playlist più riprodotti in \(.applicationName)",
                    // Swedish
                    "Spela mest spelade i \(.applicationName)",
                    "Starta mest spelade spellista i \(.applicationName)",
                    // Norwegian
                    "Spill mest spilte i \(.applicationName)",
                    "Start mest spilte spilleliste i \(.applicationName)",
                    // Portuguese
                    "Reproduzir mais tocadas em \(.applicationName)",
                    "Iniciar lista mais tocadas em \(.applicationName)",
                    // Russian
                    "Воспроизвести самые популярные в \(.applicationName)",
                    "Запустить плейлист самых популярных в \(.applicationName)",
                    // Arabic
                    "تشغيل الأكثر تشغيلاً في \(.applicationName)",
                    "ابدأ قائمة الأكثر تشغيلاً في \(.applicationName)",
                    // Hindi
                    "\(.applicationName) में सबसे ज्यादा चलाए गए चलाएं",
                    "\(.applicationName) में सबसे लोकप्रिय प्लेलिस्ट शुरू करें",
                    // Japanese
                    "\(.applicationName)で最も再生された曲を再生",
                    "\(.applicationName)の人気曲を再生",
                    // Chinese
                    "在\(.applicationName)中播放最常播放",
                    "播放\(.applicationName)的热门歌曲",
                    // Danish
                    "Afspil mest spillede i \(.applicationName)",
                    "Start mest spillede playliste i \(.applicationName)",
                    // Dutch
                    "Speel meest gespeelde in \(.applicationName)",
                    "Start meest gespeelde afspeellijst in \(.applicationName)",
                    // Korean
                    "\(.applicationName)에서 가장 많이 재생된 곡 재생",
                    "\(.applicationName)에서 인기곡 재생",
                    // Czech
                    "Přehrát nejhranější v \(.applicationName)",
                    "Spustit playlist nejhranějších v \(.applicationName)",
                    // Polish
                    "Odtwórz najczęściej odtwarzane w \(.applicationName)",
                    "Uruchom playlistę najczęściej odtwarzanych w \(.applicationName)",
                    // Finnish
                    "Toista eniten toistetut \(.applicationName)ssa",
                    "Käynnistä eniten toistettujen soittolista \(.applicationName)ssa"
                ],
                shortTitle: "Play most played music",
                systemImageName: "play.fill"
            ),
            AppShortcut(
                intent: ToggleShuffleIntent(),
                phrases: [
                    // English
                    "Toggle shuffle mode in \(.applicationName)",
                    "Change shuffle mode in \(.applicationName)",
                    // French
                    "Change le mode aléatoire dans \(.applicationName)",
                    "Active le shuffle dans \(.applicationName)",
                    // German
                    "Zufallsmodus umschalten in \(.applicationName)",
                    "Shuffle-Modus ändern in \(.applicationName)",
                    // Spanish
                    "Cambiar modo aleatorio en \(.applicationName)",
                    "Activar shuffle en \(.applicationName)",
                    // Italian
                    "Attiva modalità casuale in \(.applicationName)",
                    "Cambia shuffle in \(.applicationName)",
                    // Swedish
                    "Växla blandningsläge i \(.applicationName)",
                    "Ändra shuffle i \(.applicationName)",
                    // Norwegian
                    "Bytt tilfeldig modus i \(.applicationName)",
                    "Endre shuffle i \(.applicationName)",
                    // Portuguese
                    "Alternar modo aleatório em \(.applicationName)",
                    "Mudar shuffle em \(.applicationName)",
                    // Russian
                    "Переключить случайный режим в \(.applicationName)",
                    "Изменить режим перемешивания в \(.applicationName)",
                    // Arabic
                    "تبديل وضع العشوائية في \(.applicationName)",
                    "تغيير وضع الخلط في \(.applicationName)",
                    // Hindi
                    "\(.applicationName) में शफल मोड बदलें",
                    "\(.applicationName) में रैंडम मोड टॉगल करें",
                    // Japanese
                    "\(.applicationName)のシャッフルモードを切り替え",
                    "\(.applicationName)のランダム再生を変更",
                    // Chinese
                    "在\(.applicationName)中切换随机模式",
                    "更改\(.applicationName)的随机播放",
                    // Danish
                    "Skift shuffle-tilstand i \(.applicationName)",
                    "Ændre tilfældig afspilning i \(.applicationName)",
                    // Dutch
                    "Schakel shuffle-modus in \(.applicationName)",
                    "Wijzig willekeurige weergave in \(.applicationName)",
                    // Korean
                    "\(.applicationName)에서 셔플 모드 전환",
                    "\(.applicationName)에서 무작위 재생 변경",
                    // Czech
                    "Přepnout náhodný režim v \(.applicationName)",
                    "Změnit shuffle v \(.applicationName)",
                    // Polish
                    "Przełącz tryb losowy w \(.applicationName)",
                    "Zmień shuffle w \(.applicationName)",
                    // Finnish
                    "Vaihda sekoitustila \(.applicationName)ssa",
                    "Muuta satunnaistoistoa \(.applicationName)ssa"
                ],
                shortTitle: "Toggle Shuffle",
                systemImageName: "shuffle"
            ),
            AppShortcut(
                intent: ToggleLoopModeIntent(),
                phrases: [
                    // English
                    "Change loop mode in \(.applicationName)",
                    "Toggle repeat in \(.applicationName)",
                    // French
                    "Change le mode répétition dans \(.applicationName)",
                    "Active la répétition dans \(.applicationName)",
                    // German
                    "Wiederholungsmodus ändern in \(.applicationName)",
                    "Repeat umschalten in \(.applicationName)",
                    // Spanish
                    "Cambiar modo de repetición en \(.applicationName)",
                    "Activar repetición en \(.applicationName)",
                    // Italian
                    "Cambia modalità ripetizione in \(.applicationName)",
                    "Attiva ripeti in \(.applicationName)",
                    // Swedish
                    "Ändra upprepningsläge i \(.applicationName)",
                    "Växla upprepning i \(.applicationName)",
                    // Norwegian
                    "Endre gjentakelsesmodus i \(.applicationName)",
                    "Bytt repetisjon i \(.applicationName)",
                    // Portuguese
                    "Mudar modo de repetição em \(.applicationName)",
                    "Alternar repetição em \(.applicationName)",
                    // Russian
                    "Изменить режим повтора в \(.applicationName)",
                    "Переключить повтор в \(.applicationName)",
                    // Arabic
                    "تغيير وضع التكرار في \(.applicationName)",
                    "تبديل التكرار في \(.applicationName)",
                    // Hindi
                    "\(.applicationName) में लूप मोड बदलें",
                    "\(.applicationName) में रिपीट टॉगल करें",
                    // Japanese
                    "\(.applicationName)のリピートモードを変更",
                    "\(.applicationName)の繰り返しを切り替え",
                    // Chinese
                    "在\(.applicationName)中更改循环模式",
                    "切换\(.applicationName)的重复播放",
                    // Danish
                    "Skift gentagelsestilstand i \(.applicationName)",
                    "Ændre gentag i \(.applicationName)",
                    // Dutch
                    "Verander herhaalmodus in \(.applicationName)",
                    "Schakel herhaling in \(.applicationName)",
                    // Korean
                    "\(.applicationName)에서 반복 모드 변경",
                    "\(.applicationName)에서 반복 재생 전환",
                    // Czech
                    "Změnit režim opakování v \(.applicationName)",
                    "Přepnout opakování v \(.applicationName)",
                    // Polish
                    "Zmień tryb powtarzania w \(.applicationName)",
                    "Przełącz powtarzanie w \(.applicationName)",
                    // Finnish
                    "Muuta toistotilaa \(.applicationName)ssa",
                    "Vaihda toistoa \(.applicationName)ssa"
                ],
                shortTitle: "Toggle Loop Mode",
                systemImageName: "repeat"
            ),
            AppShortcut(
                intent: GetNowPlayingIntent(),
                phrases: [
                    // English
                    "What's playing in \(.applicationName)",
                    "Current song in \(.applicationName)",
                    "Now playing in \(.applicationName)",
                    // French
                    "Morceau en cours dans \(.applicationName)",
                    "Titre en cours dans \(.applicationName)",
                    // German
                    "Was spielt in \(.applicationName)",
                    "Aktueller Song in \(.applicationName)",
                    // Spanish
                    "Qué se está reproduciendo en \(.applicationName)",
                    "Canción actual en \(.applicationName)",
                    // Italian
                    "Cosa sta suonando in \(.applicationName)",
                    "Brano corrente in \(.applicationName)",
                    // Swedish
                    "Vad spelar i \(.applicationName)",
                    "Nuvarande låt i \(.applicationName)",
                    // Norwegian
                    "Hva spiller i \(.applicationName)",
                    "Nåværende sang i \(.applicationName)",
                    // Portuguese
                    "O que está tocando em \(.applicationName)",
                    "Música atual em \(.applicationName)",
                    // Russian
                    "Что играет в \(.applicationName)",
                    "Текущая песня в \(.applicationName)",
                    // Arabic
                    "ما الذي يتم تشغيله في \(.applicationName)",
                    "الأغنية الحالية في \(.applicationName)",
                    // Hindi
                    "\(.applicationName) में क्या चल रहा है",
                    "\(.applicationName) में वर्तमान गाना",
                    // Japanese
                    "\(.applicationName)で再生中の曲",
                    "\(.applicationName)の現在の曲",
                    // Chinese
                    "\(.applicationName)正在播放什么",
                    "\(.applicationName)的当前歌曲",
                    // Danish
                    "Hvad afspiller i \(.applicationName)",
                    "Nuværende sang i \(.applicationName)",
                    // Dutch
                    "Wat speelt er in \(.applicationName)",
                    "Huidige nummer in \(.applicationName)",
                    // Korean
                    "\(.applicationName)에서 재생 중인 곡",
                    "\(.applicationName)의 현재 곡",
                    // Czech
                    "Co se přehrává v \(.applicationName)",
                    "Aktuální skladba v \(.applicationName)",
                    // Polish
                    "Co gra w \(.applicationName)",
                    "Bieżący utwór w \(.applicationName)",
                    // Finnish
                    "Mitä soitetaan \(.applicationName)ssa",
                    "Nykyinen kappale \(.applicationName)ssa"
                ],
                shortTitle: "Now Playing",
                systemImageName: "music.note"
            )
        ]
    }
}


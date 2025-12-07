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
        guard let bridgeClass = NSClassFromString("ModizerPlaylistBridge") as? NSObject.Type,
              let bridge = bridgeClass.perform(NSSelectorFromString("sharedInstance"))?.takeUnretainedValue() as? NSObject else {
            return false
        }

        let selector = NSSelectorFromString("playPlaylistWithId:startIndex:")
        guard bridge.responds(to: selector) else { return false }

        let method = bridge.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector, Int32, Int32) -> Bool
        let implementation = unsafeBitCast(method, to: MethodType.self)

        return implementation(bridge, selector, Int32(playlistId), Int32(startIndex))
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
    private func findBestMatchingPlaylist(for searchName: String) -> (id: Int, name: String, size: Int)? {
        let playlists = getAvailablePlaylists()

        guard !playlists.isEmpty else { return nil }

        // Normaliser la recherche (nombres → chiffres, puis minuscules, sans accents)
        let withNumbers = normalizeNumbers(in: searchName)
        let normalizedSearch = withNumbers.folding(options: [.diacriticInsensitive, .caseInsensitive], locale: .current)

        // 1. Essai exact match (insensible à la casse, accents et nombres)
        if let exactMatch = playlists.first(where: {
            let playlistNormalized = normalizeNumbers(in: $0.name)
                .folding(options: [.diacriticInsensitive, .caseInsensitive], locale: .current)
            return playlistNormalized == normalizedSearch
        }) {
            return exactMatch
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
    static var typeDisplayRepresentation: TypeDisplayRepresentation { TypeDisplayRepresentation(name: "Playlist") }

    @MainActor
    struct Query: EntityQuery {
        func entities(for identifiers: [PlaylistEntity.ID]) async throws -> [PlaylistEntity] {
            let playlists = ModizerPlayerBridge.shared.getAvailablePlaylists()
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
    static let description = IntentDescription("Starts playing music in Modizer.")
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
    static let description = IntentDescription("Pauses music playback in Modizer.")
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
    static let description = IntentDescription("Stops music playback in Modizer.")
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
    static let description = IntentDescription("Skips to the next song in Modizer.")
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
    static let description = IntentDescription("Skips to the previous song in Modizer.")
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
    static let description = IntentDescription("Plays a specific subsong in the current file.")
    static let openAppWhenRun: Bool = false

    @Parameter(title: "Subsong Index", description: "The index of the subsong to play", default: 0)
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
    static let description = IntentDescription("Seeks to a specific position in the current song.")
    static let openAppWhenRun: Bool = false

    @Parameter(title: "Position (seconds)", description: "The position to seek to in seconds")
    var position: Int

    @Parameter(title: "Relative", description: "Whether to seek relative to current position", default: false)
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
    static let description = IntentDescription("Set the infinite loop mode in Modizer.")
    static let openAppWhenRun: Bool = false

    @Parameter(title: "Infinite Loop Mode", description: "0: off, 1: infinite loop", default: 1)
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
    static let description = IntentDescription("Set the loop mode in Modizer.")
    static let openAppWhenRun: Bool = false

    @Parameter(title: "Repeat Mode", description: "0: off, 1: loop current title, 2: loop playlist", default: 1)
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
    static let description = IntentDescription("Set the shuffle mode in Modizer.")
    static let openAppWhenRun: Bool = false

    @Parameter(title: "Shuffle Mode", description: "0: off, 1: choose only one title if file has subsongs or is an archive, 2: play all titles of a given file", default: 1)
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
    static let description = IntentDescription("Toggles shuffle mode in Modizer.")
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
    static let description = IntentDescription("Toggles loop mode in Modizer.")
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
    static let description = IntentDescription("Set Fullscreen mode on or off in Modizer.")
    static let openAppWhenRun: Bool = true
    
    @Parameter(title: "Fullscreen mode", description: "Activate (1) or disactivate (0) Fullscreen mode", default: 1)
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
    static let description = IntentDescription("Go the player view in Modizer.")
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
    static let description = IntentDescription("Set FX mode in Modizer.")
    static let openAppWhenRun: Bool = true
    
    @Parameter(
        title: "FX",
        description: "The FX to set",
        optionsProvider: FXOptionsProvider()
    )
    var fxName: String

    @Parameter(title: "FX Mode", description: "The mode to activate for the selected FX (0:off)", default: 0)
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
            "ProjectM",
            "Oscillo",
            "Piano roll",
            "3D Piano",
            "Notes bars",
            "Tracker",
            "2D Spectrum",
            "3D Spectrum",
            "3D Landscape"
        ]
    func results() async throws -> [String] {
            return FXOptionsProvider.fxList
        }
}


// MARK: - Get Now Playing Intent
@available(iOS 16.0, *)
struct GetNowPlayingIntent: AppIntent {
    static let title: LocalizedStringResource = "Get Now Playing"
    static let description = IntentDescription("Gets information about the currently playing song.")
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
    static let description = IntentDescription("Gets the list of available playlists in Modizer.")
    static let openAppWhenRun: Bool = false

    @MainActor
    func perform() async throws -> some IntentResult & ReturnsValue<String> {
        let playlists = ModizerPlayerBridge.shared.getAvailablePlaylists()

        if playlists.isEmpty {
            return .result(value: "No playlists found")
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
    static let description = IntentDescription("Random picks from available local titles in Modizer.")
    static let openAppWhenRun: Bool = false

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.playBuiltinPlaylist(playlistId: -1)
        return .result()
    }
}

// MARK: - Play most played Playlist Intent
@available(iOS 16.0, *)
struct PlayMostPlayedIntent: AppIntent {
    static let title: LocalizedStringResource = "Play most played"
    static let description = IntentDescription("Most played titles from available local titles in Modizer.")
    static let openAppWhenRun: Bool = false

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.playBuiltinPlaylist(playlistId: -2)
        return .result()
    }
}

// MARK: - Play favorites Playlist Intent
@available(iOS 16.0, *)
struct PlayFavoritesIntent: AppIntent {
    static let title: LocalizedStringResource = "Play favorites"
    static let description = IntentDescription("Favorites titles from available local titles in Modizer.")
    static let openAppWhenRun: Bool = false

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.playBuiltinPlaylist(playlistId: -3)
        return .result()
    }
}

// MARK: - Play Playlist Intent (with entity - for Shortcuts app)
@available(iOS 16.0, *)
struct PlayPlaylistIntent: AppIntent {
    static let title: LocalizedStringResource = "Play Playlist"
    static let description = IntentDescription("Plays a specific playlist in Modizer.")
    static let openAppWhenRun: Bool = true

    static var parameterSummary: some ParameterSummary {
        Summary("Play \(\.$playlist)") {
            \.$startIndex
        }
    }

    @Parameter(
        title: "Playlist",
        description: "The playlist to play"
    )
    var playlist: PlaylistEntity

    @Parameter(title: "Start Index", description: "The index of the song to start playing from (0-based)", default: 0)
    var startIndex: Int

    @MainActor
    func perform() async throws -> some IntentResult & ProvidesDialog {
        // Use playlist id if available, otherwise fall back to name lookup
        let idInt = Int(playlist.id)
        let success: Bool
        if let pid = idInt {
            success = ModizerPlayerBridge.shared.playPlaylist(playlistId: pid, startIndex: startIndex)
        } else {
            success = ModizerPlayerBridge.shared.playPlaylist(playlistName: playlist.name, startIndex: startIndex)
        }

        if success {
            let fmt = NSLocalizedString("play_playlist_dialog", tableName: nil, bundle: .main, value: "Lecture de la playlist %@", comment: "Dialog confirming playlist playback")
            let dialog = String(format: fmt, playlist.name)
            return .result(dialog: IntentDialog(stringLiteral: dialog))
        } else {
            let fmt = NSLocalizedString("playlist_not_found_dialog", tableName: nil, bundle: .main, value: "Je n'ai pas trouvé la playlist %@", comment: "Dialog when playlist not found")
            let dialog = String(format: fmt, playlist.name)
            return .result(dialog: IntentDialog(stringLiteral: dialog))
        }
    }
}

// MARK: - Play Playlist By Name Intent (with string - for Siri)
@available(iOS 16.0, *)
struct PlayPlaylistByNameIntent: AppIntent {
    static let title: LocalizedStringResource = "Play Playlist By Name"
    static let description = IntentDescription("Plays a playlist by name in Modizer.")
    static let openAppWhenRun: Bool = true

    static var parameterSummary: some ParameterSummary {
        Summary("Play playlist \(\.$playlistName)")
    }

    @Parameter(
        title: "Playlist Name",
        description: "The name of the playlist to play",
        requestValueDialog: IntentDialog(LocalizedStringResource("Which playlist would you like to play?"))
    )
    var playlistName: String

    @Parameter(title: "Start Index", description: "The index of the song to start playing from (0-based)", default: 0)
    var startIndex: Int

    @MainActor
    func perform() async throws -> some IntentResult & ProvidesDialog {
        let success = ModizerPlayerBridge.shared.playPlaylist(playlistName: playlistName, startIndex: startIndex)

        if success {
            let fmt = NSLocalizedString("play_playlist_dialog", tableName: nil, bundle: .main, value: "Lecture de la playlist %@", comment: "Dialog confirming playlist playback")
            let dialog = String(format: fmt, playlistName)
            return .result(dialog: IntentDialog(stringLiteral: dialog))
        } else {
            let fmt = NSLocalizedString("playlist_not_found_dialog", tableName: nil, bundle: .main, value: "Je n'ai pas trouvé la playlist %@", comment: "Dialog when playlist not found")
            let dialog = String(format: fmt, playlistName)
            return .result(dialog: IntentDialog(stringLiteral: dialog))
        }
    }
}

@available(iOS 16.0, *)
enum PlaylistError: Error, CustomLocalizedStringResourceConvertible {
    case playlistNotFound

    var localizedStringResource: LocalizedStringResource {
        switch self {
        case .playlistNotFound:
            return "Playlist not found. Use 'Get Playlists' to see available playlists."
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
                    "Joue la playlist dans \(.applicationName)",
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
                    "播放\(.applicationName)的播放列表"
                ],
                shortTitle: "Play Playlist",
                systemImageName: "music.note.list"
            ),
//            AppShortcut(
//                intent: PlayIntent(),
//                phrases: [
//                    // English
//                    "Play in \(.applicationName)",
//                    "Start playing \(.applicationName)",
//                    "Resume \(.applicationName)",
//                    // French
//                    "Lecture dans \(.applicationName)",
//                    "Reprend la lecture dans \(.applicationName)",
//                    // German
//                    "Spiele in \(.applicationName)",
//                    "Starte Wiedergabe in \(.applicationName)",
//                    // Spanish
//                    "Reproduce en \(.applicationName)",
//                    "Iniciar reproducción en \(.applicationName)",
//                    // Italian
//                    "Riproduci in \(.applicationName)",
//                    "Avvia riproduzione in \(.applicationName)",
//                    // Swedish
//                    "Spela i \(.applicationName)",
//                    "Starta uppspelning i \(.applicationName)",
//                    // Norwegian
//                    "Spill i \(.applicationName)",
//                    "Start avspilling i \(.applicationName)",
//                    // Portuguese
//                    "Reproduzir em \(.applicationName)",
//                    "Iniciar reprodução em \(.applicationName)",
//                    // Russian
//                    "Воспроизвести в \(.applicationName)",
//                    "Запустить воспроизведение в \(.applicationName)",
//                    // Arabic
//                    "تشغيل في \(.applicationName)",
//                    "ابدأ التشغيل في \(.applicationName)",
//                    // Hindi
//                    "\(.applicationName) में चलाएं",
//                    "\(.applicationName) में प्लेबैक शुरू करें",
//                    // Japanese
//                    "\(.applicationName)で再生",
//                    "\(.applicationName)を再生して",
//                    // Chinese
//                    "在\(.applicationName)中播放",
//                    "播放\(.applicationName)"
//                ],
//                shortTitle: "Play",
//                systemImageName: "play.fill"
//            ),
            AppShortcut(
                intent: PlayRandomPicksIntent(),
                phrases: [
                    // English
                    "Play some music in \(.applicationName)",
                    "Start random playlist in \(.applicationName)",
                    // French
                    "Joue de la musique dans \(.applicationName)",
                    "Lance la playlist aléatoire dans \(.applicationName)",
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
                ],
                shortTitle: "Play most played music",
                systemImageName: "play.fill"
            ),
//            AppShortcut(
//                intent: PauseIntent(),
//                phrases: [
//                    // English
//                    "Pause \(.applicationName)",
//                    "Pause music in \(.applicationName)",
//                    // French
//                    "Pause dans \(.applicationName)",
//                    "Pause la musique dans \(.applicationName)",
//                    // German
//                    "Pausiere in \(.applicationName)",
//                    "Pause Musik in \(.applicationName)",
//                    // Spanish
//                    "Pausa en \(.applicationName)",
//                    "Pausar música en \(.applicationName)",
//                    // Italian
//                    "Pausa in \(.applicationName)",
//                    "Metti in pausa in \(.applicationName)",
//                    // Swedish
//                    "Pausa i \(.applicationName)",
//                    "Pausa musik i \(.applicationName)",
//                    // Norwegian
//                    "Pause i \(.applicationName)",
//                    "Pause musikk i \(.applicationName)",
//                    // Portuguese
//                    "Pausar em \(.applicationName)",
//                    "Pausar música em \(.applicationName)",
//                    // Russian
//                    "Пауза в \(.applicationName)",
//                    "Приостановить музыку в \(.applicationName)",
//                    // Arabic
//                    "إيقاف مؤقت في \(.applicationName)",
//                    "إيقاف الموسيقى مؤقتًا في \(.applicationName)",
//                    // Hindi
//                    "\(.applicationName) में रोकें",
//                    "\(.applicationName) में संगीत रोकें",
//                    // Japanese
//                    "\(.applicationName)を一時停止",
//                    "\(.applicationName)で一時停止して",
//                    // Chinese
//                    "暂停\(.applicationName)",
//                    "在\(.applicationName)中暂停"
//                ],
//                shortTitle: "Pause",
//                systemImageName: "pause.fill"
//            ),
//            AppShortcut(
//                intent: StopIntent(),
//                phrases: [
//                    // English
//                    "Stop \(.applicationName)",
//                    "Stop music in \(.applicationName)",
//                    // French
//                    "Stop \(.applicationName)",
//                    "Stop la musique dans \(.applicationName)",
//                    // German
//                    "Stoppe in \(.applicationName)",
//                    "Musik stoppen in \(.applicationName)",
//                    // Spanish
//                    "Detener en \(.applicationName)",
//                    "Detener música en \(.applicationName)",
//                    // Italian
//                    "Ferma in \(.applicationName)",
//                    "Ferma la musica in \(.applicationName)",
//                    // Swedish
//                    "Stoppa i \(.applicationName)",
//                    "Stoppa musik i \(.applicationName)",
//                    // Norwegian
//                    "Stopp i \(.applicationName)",
//                    "Stopp musikk i \(.applicationName)",
//                    // Portuguese
//                    "Parar em \(.applicationName)",
//                    "Parar música em \(.applicationName)",
//                    // Russian
//                    "Остановить в \(.applicationName)",
//                    "Остановить музыку в \(.applicationName)",
//                    // Arabic
//                    "إيقاف في \(.applicationName)",
//                    "إيقاف الموسيقى في \(.applicationName)",
//                    // Hindi
//                    "\(.applicationName) में रुकें",
//                    "\(.applicationName) में संगीत रोकें",
//                    // Japanese
//                    "\(.applicationName)を停止",
//                    "\(.applicationName)で音楽を停止",
//                    // Chinese
//                    "停止\(.applicationName)",
//                    "在\(.applicationName)中停止音乐"
//                ],
//                shortTitle: "Stop",
//                systemImageName: "stop.fill"
//            ),
//            AppShortcut(
//                intent: NextSongIntent(),
//                phrases: [
//                    // English
//                    "Next song in \(.applicationName)",
//                    "Skip to next in \(.applicationName)",
//                    "Play next in \(.applicationName)",
//                    // French
//                    "Morceau suivant dans \(.applicationName)",
//                    "Piste suivante dans \(.applicationName)",
//                    // German
//                    "Nächstes Lied in \(.applicationName)",
//                    "Weiter in \(.applicationName)",
//                    // Spanish
//                    "Siguiente canción en \(.applicationName)",
//                    "Saltar a siguiente en \(.applicationName)",
//                    // Italian
//                    "Prossimo brano in \(.applicationName)",
//                    "Avanti in \(.applicationName)",
//                    // Swedish
//                    "Nästa låt i \(.applicationName)",
//                    "Hoppa till nästa i \(.applicationName)",
//                    // Norwegian
//                    "Neste sang i \(.applicationName)",
//                    "Hopp til neste i \(.applicationName)",
//                    // Portuguese
//                    "Próxima música em \(.applicationName)",
//                    "Pular para próxima em \(.applicationName)",
//                    // Russian
//                    "Следующая песня в \(.applicationName)",
//                    "Перейти к следующей в \(.applicationName)",
//                    // Arabic
//                    "الأغنية التالية في \(.applicationName)",
//                    "انتقل إلى التالي في \(.applicationName)",
//                    // Hindi
//                    "\(.applicationName) में अगला गाना",
//                    "\(.applicationName) में अगले पर जाएं",
//                    // Japanese
//                    "\(.applicationName)で次の曲",
//                    "\(.applicationName)で次へ",
//                    // Chinese
//                    "\(.applicationName)的下一首",
//                    "在\(.applicationName)中播放下一首"
//                ],
//                shortTitle: "Next Song",
//                systemImageName: "forward.fill"
//            ),
//            AppShortcut(
//                intent: PreviousSongIntent(),
//                phrases: [
//                    // English
//                    "Previous song in \(.applicationName)",
//                    "Go back in \(.applicationName)",
//                    "Play previous in \(.applicationName)",
//                    // French
//                    "Morceau précédent dans \(.applicationName)",
//                    "Piste précédente dans \(.applicationName)",
//                    // German
//                    "Vorheriges Lied in \(.applicationName)",
//                    "Zurück in \(.applicationName)",
//                    // Spanish
//                    "Canción anterior en \(.applicationName)",
//                    "Volver atrás en \(.applicationName)",
//                    // Italian
//                    "Brano precedente in \(.applicationName)",
//                    "Indietro in \(.applicationName)",
//                    // Swedish
//                    "Föregående låt i \(.applicationName)",
//                    "Gå tillbaka i \(.applicationName)",
//                    // Norwegian
//                    "Forrige sang i \(.applicationName)",
//                    "Gå tilbake i \(.applicationName)",
//                    // Portuguese
//                    "Música anterior em \(.applicationName)",
//                    "Voltar em \(.applicationName)",
//                    // Russian
//                    "Предыдущая песня в \(.applicationName)",
//                    "Перейти к предыдущей в \(.applicationName)",
//                    // Arabic
//                    "الأغنية السابقة في \(.applicationName)",
//                    "العودة في \(.applicationName)",
//                    // Hindi
//                    "\(.applicationName) में पिछला गाना",
//                    "\(.applicationName) में वापस जाएं",
//                    // Japanese
//                    "\(.applicationName)で前の曲",
//                    "\(.applicationName)で前へ",
//                    // Chinese
//                    "\(.applicationName)的上一首",
//                    "在\(.applicationName)中播放上一首"
//                ],
//                shortTitle: "Previous Song",
//                systemImageName: "backward.fill"
//            ),
            AppShortcut(
                intent: ToggleShuffleIntent(),
                phrases: [
                    //English
                    "Toggle shuffle mode in \(.applicationName)",
                    "Change shuffle mode in \(.applicationName)",
                    //French
                    "Change le mode aléatoire dans \(.applicationName)",
                    "Change le shuffle mode dans \(.applicationName)"
                ],
                shortTitle: "Toggle Shuffle",
                systemImageName: "shuffle"
            ),
            AppShortcut(
                intent: ToggleLoopModeIntent(),
                phrases: [
                    //English
                    "Change loop mode in \(.applicationName)",
                    //French
                    "Change le mode répétition dans \(.applicationName)",
                ],
                shortTitle: "Toggle Loop Mode",
                systemImageName: "repeat"
            ),
            AppShortcut(
                intent: GetNowPlayingIntent(),
                phrases: [
                    //English
                    "What's playing in \(.applicationName)",
                    "Current song in \(.applicationName)",
                    "Now playing in \(.applicationName)",
                    //French
                    "Morceau en cours dans \(.applicationName)",
                    "Titre en cours dans \(.applicationName)"
                ],
                shortTitle: "Now Playing",
                systemImageName: "music.note"
            )
        ]
    }
}


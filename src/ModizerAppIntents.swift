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

    private var musicPlayer: AnyObject? {
        guard let detailVC = detailViewController as? NSObject,
              let player = detailVC.value(forKey: "mplayer") as? NSObject else {
            return nil
        }
        return player
    }

    func play() {
        musicPlayer?.perform(NSSelectorFromString("Play"))
    }

    func pause(_ paused: Bool) {
        guard let player = musicPlayer as? NSObject else { return }

        let selector = NSSelectorFromString("Pause:")
        guard player.responds(to: selector) else { return }

        let method = player.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector, Bool) -> Void
        let implementation = unsafeBitCast(method, to: MethodType.self)
        implementation(player, selector, paused)
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

    func playPlaylist(playlistName: String, startIndex: Int = 0) -> Bool {
        guard let bridgeClass = NSClassFromString("ModizerPlaylistBridge") as? NSObject.Type,
              let bridge = bridgeClass.perform(NSSelectorFromString("sharedInstance"))?.takeUnretainedValue() as? NSObject else {
            return false
        }

        let selector = NSSelectorFromString("playPlaylistWithName:startIndex:")
        guard bridge.responds(to: selector) else { return false }

        let method = bridge.method(for: selector)
        typealias MethodType = @convention(c) (NSObject, Selector, NSString, Int32) -> Bool
        let implementation = unsafeBitCast(method, to: MethodType.self)

        return implementation(bridge, selector, playlistName as NSString, Int32(startIndex))
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
        ModizerPlayerBridge.shared.pause(true)
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

// MARK: - Get Now Playing Intent
@available(iOS 16.0, *)
struct GetNowPlayingIntent: AppIntent {
    static let title: LocalizedStringResource = "Get Now Playing"
    static let description = IntentDescription("Gets information about the currently playing song.")
    static let openAppWhenRun: Bool = false

    @MainActor
    func perform() async throws -> some IntentResult & ReturnsValue<String> {
        let title = ModizerPlayerBridge.shared.getCurrentSongTitle()
        let isPlaying = ModizerPlayerBridge.shared.isPlaying()
        let currentTime = ModizerPlayerBridge.shared.getCurrentTime()
        let totalTime = ModizerPlayerBridge.shared.getSongLength()

        let status = isPlaying ? "Playing" : "Paused"
        let info = """
        \(status): \(title)
        Time: \(currentTime)s / \(totalTime)s
        """

        return .result(value: info)
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

// MARK: - Play Playlist Intent
@available(iOS 16.0, *)
struct PlayPlaylistIntent: AppIntent {
    static let title: LocalizedStringResource = "Play Playlist"
    static let description = IntentDescription("Plays a specific playlist in Modizer.")
    static let openAppWhenRun: Bool = true

    @Parameter(title: "Playlist Name", description: "The name of the playlist to play")
    var playlistName: String

    @Parameter(title: "Start Index", description: "The index of the song to start playing from (0-based)", default: 0)
    var startIndex: Int

    @MainActor
    func perform() async throws -> some IntentResult {
        let success = ModizerPlayerBridge.shared.playPlaylist(playlistName: playlistName, startIndex: startIndex)

        if success {
            return .result()
        } else {
            throw PlaylistError.playlistNotFound
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
                intent: PlayIntent(),
                phrases: [
                    "Play in \(.applicationName)",
                    "Start playing \(.applicationName)",
                    "Resume \(.applicationName)"
                ],
                shortTitle: "Play",
                systemImageName: "play.fill"
            ),
            AppShortcut(
                intent: PauseIntent(),
                phrases: [
                    "Pause \(.applicationName)",
                    "Pause music in \(.applicationName)"
                ],
                shortTitle: "Pause",
                systemImageName: "pause.fill"
            ),
            AppShortcut(
                intent: StopIntent(),
                phrases: [
                    "Stop \(.applicationName)",
                    "Stop music in \(.applicationName)"
                ],
                shortTitle: "Stop",
                systemImageName: "stop.fill"
            ),
            AppShortcut(
                intent: NextSongIntent(),
                phrases: [
                    "Next song in \(.applicationName)",
                    "Skip to next in \(.applicationName)",
                    "Play next in \(.applicationName)"
                ],
                shortTitle: "Next Song",
                systemImageName: "forward.fill"
            ),
            AppShortcut(
                intent: PreviousSongIntent(),
                phrases: [
                    "Previous song in \(.applicationName)",
                    "Go back in \(.applicationName)",
                    "Play previous in \(.applicationName)"
                ],
                shortTitle: "Previous Song",
                systemImageName: "backward.fill"
            ),
            AppShortcut(
                intent: GetNowPlayingIntent(),
                phrases: [
                    "What's playing in \(.applicationName)",
                    "Current song in \(.applicationName)",
                    "Now playing in \(.applicationName)"
                ],
                shortTitle: "Now Playing",
                systemImageName: "music.note"
            ),
            AppShortcut(
                intent: GetPlaylistsIntent(),
                phrases: [
                    "Show my playlists in \(.applicationName)",
                    "List playlists in \(.applicationName)",
                    "Get playlists from \(.applicationName)"
                ],
                shortTitle: "Get Playlists",
                systemImageName: "music.note.list"
            )
        ]
    }
}

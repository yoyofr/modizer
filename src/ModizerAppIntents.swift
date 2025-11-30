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
        musicPlayer?.perform(NSSelectorFromString("Pause:"), with: paused)
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
        musicPlayer?.perform(NSSelectorFromString("playGoToSub:"), with: Int32(index))
    }

    func seek(to position: Int64) {
        let positionValue = NSNumber(value: position)
        musicPlayer?.perform(NSSelectorFromString("Seek:"), with: positionValue)
    }

    func getCurrentTime() -> Int {
        guard let player = musicPlayer as? NSObject,
              let result = player.perform(NSSelectorFromString("getCurrentTime")) else {
            return 0
        }
        return Int(result.toOpaque().assumingMemoryBound(to: Int32.self).pointee)
    }

    func getSongLength() -> Int {
        guard let player = musicPlayer as? NSObject,
              let result = player.perform(NSSelectorFromString("getSongLength")) else {
            return 0
        }
        return Int(result.toOpaque().assumingMemoryBound(to: Int32.self).pointee)
    }

    func getCurrentSongTitle() -> String {
        guard let player = musicPlayer as? NSObject,
              let result = player.perform(NSSelectorFromString("getModFileTitleOrNull")),
              let title = result.takeUnretainedValue() as? String else {
            return "Unknown"
        }
        return title
    }

    func isPlaying() -> Bool {
        guard let player = musicPlayer as? NSObject,
              let result = player.perform(NSSelectorFromString("isPlaying")) else {
            return false
        }
        return result.toOpaque().assumingMemoryBound(to: Bool.self).pointee
    }

    func setLoopMode(_ mode: Int) {
        musicPlayer?.perform(NSSelectorFromString("setLoopInf:"), with: Int32(mode))
    }

    func toggleShuffle() {
        detailViewController?.perform(NSSelectorFromString("shuffle"))
    }

    func getShuffleState() -> Int {
        guard let detailVC = detailViewController as? NSObject,
              let shuffleState = detailVC.value(forKey: "mShuffle") as? Int32 else {
            return 0
        }
        return Int(shuffleState)
    }
}

// MARK: - Play Intent
@available(iOS 16.0, *)
struct PlayIntent: AppIntent {
    static let title: LocalizedStringResource = "Play"
    static let description = IntentDescription("Starts playing music in Modizer.")
    static let openAppWhenRun: Bool = true

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

// MARK: - Toggle Repeat Intent
@available(iOS 16.0, *)
struct ToggleRepeatIntent: AppIntent {
    static let title: LocalizedStringResource = "Toggle Repeat"
    static let description = IntentDescription("Toggles repeat/loop mode in Modizer.")
    static let openAppWhenRun: Bool = false

    @Parameter(title: "Repeat Mode", description: "0: off, 1: infinite loop", default: 1)
    var mode: Int

    @MainActor
    func perform() async throws -> some IntentResult {
        ModizerPlayerBridge.shared.setLoopMode(mode)
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

// MARK: - Play Playlist Intent
@available(iOS 16.0, *)
struct PlayPlaylistIntent: AppIntent {
    static let title: LocalizedStringResource = "Play Playlist"
    static let description = IntentDescription("Plays a specific playlist in Modizer.")
    static let openAppWhenRun: Bool = true

    @Parameter(title: "Playlist Name", description: "The name of the playlist to play")
    var playlistName: String

    @MainActor
    func perform() async throws -> some IntentResult {
        // This would need access to the playlist controller
        // Will need to implement based on Modizer's playlist architecture
        let activity = NSUserActivity(activityType: "com.yoyofr.modizer.playPlaylist")
        activity.userInfo = ["playlistName": playlistName]
        return .result()
    }
}

// MARK: - App Shortcuts Provider
@available(iOS 16.0, *)
struct ModizerShortcuts: AppShortcutsProvider {
    static var appShortcuts: [AppShortcut] {
        AppShortcut(
            intent: PlayIntent(),
            phrases: [
                "Play in \(.applicationName)",
                "Start playing \(.applicationName)",
                "Resume \(.applicationName)"
            ],
            shortTitle: "Play",
            systemImageName: "play.fill"
        )

        AppShortcut(
            intent: PauseIntent(),
            phrases: [
                "Pause \(.applicationName)",
                "Pause music in \(.applicationName)"
            ],
            shortTitle: "Pause",
            systemImageName: "pause.fill"
        )

        AppShortcut(
            intent: NextSongIntent(),
            phrases: [
                "Next song in \(.applicationName)",
                "Skip to next in \(.applicationName)",
                "Play next in \(.applicationName)"
            ],
            shortTitle: "Next Song",
            systemImageName: "forward.fill"
        )

        AppShortcut(
            intent: PreviousSongIntent(),
            phrases: [
                "Previous song in \(.applicationName)",
                "Go back in \(.applicationName)",
                "Play previous in \(.applicationName)"
            ],
            shortTitle: "Previous Song",
            systemImageName: "backward.fill"
        )

        AppShortcut(
            intent: GetNowPlayingIntent(),
            phrases: [
                "What's playing in \(.applicationName)",
                "Current song in \(.applicationName)",
                "Now playing in \(.applicationName)"
            ],
            shortTitle: "Now Playing",
            systemImageName: "music.note"
        )
    }
}

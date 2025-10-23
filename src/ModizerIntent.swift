//
//  ModizerIntent.swift
//  modizer
//
//  Created by Yohann Magnien David on 22/10/2025.
//
import AppIntents

@available(iOS 16.0, *)
struct OpenWithActivityIntent: AppIntent {
    static let title: LocalizedStringResource = "Open Specific Feature"
    static let description = IntentDescription("Opens the app and goes to your favorite trails.")
        
    static let openAppWhenRun: Bool = true
    
    @Parameter(title: "Feature")
    var feature: String
    
    @MainActor
    func perform() async throws -> some IntentResult {
        // Create a user activity
        let activity = NSUserActivity(activityType: "com.yoyofr.modizer.openFeature")
        activity.userInfo = ["feature": feature]
        activity.isEligibleForHandoff = true
        
        // This will trigger application:continueUserActivity:restorationHandler:
        return .result()
    }
}

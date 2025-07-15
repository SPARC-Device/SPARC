package com.sparc.notify

import android.util.Log
import com.google.firebase.messaging.FirebaseMessagingService
import com.google.firebase.messaging.RemoteMessage

class MyFirebaseMessagingService : FirebaseMessagingService() {
    override fun onNewToken(token: String) {
        super.onNewToken(token)
        Log.i("FCM", "New token: $token")
    }

    override fun onMessageReceived(remoteMessage: RemoteMessage) {
        super.onMessageReceived(remoteMessage)
        Log.i("FCM", "Message received from: ${remoteMessage.from}")
        Log.i("FCM", "Message data: ${remoteMessage.data}")
        remoteMessage.notification?.let {
            Log.i("FCM", "Notification title: ${it.title}")
            Log.i("FCM", "Notification body: ${it.body}")
        }
    }
} 
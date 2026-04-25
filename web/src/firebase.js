// src/firebase.js
import { initializeApp } from "firebase/app";
import { getDatabase } from "firebase/database";

// DİKKAT: Buraya kendi Firebase Console'dan kopyaladığın config objesini yapıştır!
const firebaseConfig = {
  apiKey: "AIzaSyB1JD-nyRlM2B_kWXxjTeg3v9Yddu4xDjc",
  authDomain: "smartsafe-8f4f9.firebaseapp.com",
  databaseURL: "https://smartsafe-8f4f9-default-rtdb.firebaseio.com",
  projectId: "smartsafe-8f4f9",
  storageBucket: "smartsafe-8f4f9.firebasestorage.app",
  messagingSenderId: "648749418907",
  appId: "1:648749418907:web:a54f48cc2d23d63e07f93e",
  measurementId: "G-1QB5SZ3QMV"
};

// Firebase'i başlat
const app = initializeApp(firebaseConfig);

// Realtime Database referansını dışarı aktar ki App.jsx bunu kullanabilsin
export const db = getDatabase(app);
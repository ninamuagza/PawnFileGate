#if defined _filegate_included
	#endinput
#endif
#define _filegate_included

/**
 * FileGate Plugin - HTTP File Receiver
 * Created for open.mp/Harwana Project
 */

// ─────────────────────────────────────────────────────────────────────────────
// Conflict Modes
// ─────────────────────────────────────────────────────────────────────────────
#define CONFLICT_RENAME    0
#define CONFLICT_OVERWRITE 1
#define CONFLICT_REJECT    2

// ─────────────────────────────────────────────────────────────────────────────
// Natives
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Mendaftarkan sebuah path HTTP POST untuk menerima file upload dari client.
 * 
 * @param endpoint The HTTP URL path (contoh: "/map")
 * @param path The server directory destination (contoh: "scriptfiles/maps/")
 * @param allowedExts CSV ekstensi yang diizinkan (contoh: ".map,.json")
 * @param maxSizeMb Maksimum ukuran file dalam Megabyte
 * @return return routeId, atau -1 jika gagal
 */
native FileReceiver_RegisterRoute(const endpoint[], const path[], const allowedExts[], maxSizeMb);

/**
 * Mewajibkan Header "Authorization: Bearer <key>" untuk endpoint tertentu.
 */
native bool:FileReceiver_AddKey(routeId, const key[]);

/**
 * Mencabut requirement Header Authorization dari endpoint tertentu.
 */
native bool:FileReceiver_RemoveKey(routeId, const key[]);

/**
 * Mengubah setelan penanganan jika nama file bentrok di destination.
 * Bisa memakai CONFLICT_RENAME (default, menamakan ulang jadi _1, _2 dst), 
 * CONFLICT_OVERWRITE, atau CONFLICT_REJECT.
 */
native bool:FileReceiver_SetConflict(routeId, mode);

/**
 * Menghapus rute HTTP server agar tidak lagi menerima file.
 */
native bool:FileReceiver_RemoveRoute(routeId);

// ─────────────────────────────────────────────────────────────────────────────
// Callbacks
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Dipanggil ketika file sukses terunggah dan ditransfer ke destination path.
 */
forward OnFileUploaded(uploadId, routeId, const endpoint[], const filename[], const filepath[]);

/**
 * Dipanggil ketika HTTP Request gagal diproses (file terlalu besar, ekstensi dilarang, token invalid, dll).
 */
forward OnFileFailedUpload(uploadId, const reason[]);

/**
 * Dipanggil secara reguler mengikuti progres upload berjalan (dalam persentase 0-100%).
 */
forward OnUploadProgress(uploadId, percent);

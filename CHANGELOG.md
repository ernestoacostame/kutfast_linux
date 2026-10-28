# Changelog

Todos los cambios notables realizados en **KutFast Linux** se documentan en este archivo.

El formato se basa en [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/).

---

## [0.2.0] - 2026-10-26

### Añadido
* **Miniaturas de Portada del Podcast en Listas y Encabezados:**
  * Visualización de la portada del podcast activo (`coverUrl`) junto al título de cada episodio en `EpisodeCard` (usado en el Panel y en Borradores).
  * Inclusión de miniatura de portada en la lista de episodios (`EpisodesPage`) y en los borradores locales (`DraftsPage`).
  * Integración de miniatura de portada en las barras de herramientas y encabezados de sección (`Episodios`, `Publicar episodio`, `Borradores en KutPod`, `Episodios recientes`).
  * Resolución automática de URLs relativas y absolutas desde la API REST de KutPod.
* **Publicación de Borradores Remotos de KutPod:**
  * Implementación del método `publishDraft(int podcastId, int episodeId)` en `KutPodClient` comunicándose con `PUT /podcasts/{podcastId}/episodes/{episodeId}` (`status: published`).
  * Conexión completa del botón *"Publicar"* en `DraftsPage` y `DashboardPage` vía `AppController::publishKutPodDraft()`.
* **Modo "Voz Rápida" (Quick Voice):**
  * Botón de atajo `⚡ + Fecha de hoy` en `PublishPage` para generar el título con la fecha actual formateada en un clic.
  * Autocompletado de descripción con fecha y hora actual si se deja en blanco al publicar.
* **Arrastrar y Soltar Global (Global Drag & Drop):**
  * Detección de archivos de audio arrastrados sobre cualquier área de la ventana principal con overlay visual interactivo (`🎙 Suelta el archivo de audio para publicarlo`).
  * Conmutación automática a la pantalla de publicación y precarga inmediata del archivo.
* **Reproductor de Audio Inline en Tarjetas de Episodios:**
  * Controles de reproducción instantánea (`▶ Escuchar` / `⏸ Pausar`) integrados en `EpisodeCard` utilizando `QtMultimedia` (`MediaPlayer` y `AudioOutput`).
* **Integración Nativa de Escritorio Linux:**
  * Soporte para recibir archivos de audio vía argumentos de línea de comandos en `main.cpp` (`kutfast-bin /ruta/audio.mp3`).
  * Actualización de la entrada `.desktop` en `install.sh` con el flag `%f` y tipos MIME (`audio/mpeg`, `audio/ogg`, `audio/flac`, `audio/x-m4a`, `audio/opus`, etc.) para permitir *"Abrir con KutFast"* desde gestores de archivos.

### Cambiado
* **Modernización Tipográfica:**
  * Configuración de pila tipográfica sans-serif moderna a nivel global (`Inter`, `Ubuntu`, `Cantarell`, `SF Pro Text`, `Segoe UI`, `Noto Sans`) en `main.cpp`.
  * Incorporación de tokens tipográficos `fontFamily`, `fontDisplay` y `fontMono` (`JetBrains Mono`, `Fira Code`) en `Theme.qml`.
  * Homogeneización del diseño en tarjetas de episodios utilizando `EpisodeCard` reutilizable en `EpisodesPage`.

---

## [0.1.0] - 2026-10-26

### Añadido
* Versión inicial nativa para GNU/Linux de KutFast desarrollada con C++, Qt 6 y QML.
* Soporte para publicación directa, grabación de audio con VU-meter y normalización LUFS con FFmpeg.
* Almacenamiento local de borradores y plantillas.

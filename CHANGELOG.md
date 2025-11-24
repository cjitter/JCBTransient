# Changelog

## 1.0.1
### Added
- Caché interna de parámetros Gen~ y helpers para sincronización segura entre APVTS y el motor DSP (rebuildGenParameterLookup, enqueueAllParametersForAudioThread, pushGenParamByName).

### Changed
- Restauración de estado y carga de presets ahora sincronizan todos los parámetros con Gen~ mediante los nuevos helpers.
- Clamps mínimos coherentes para `d_ATK` y `e_REL` aplicados en estado y presets.
- Eliminada la escritura directa a Gen~ desde UI al cargar presets (uso exclusivo de pushGenParamByName).
- Limpieza interna eliminando listeners heredados (`b_THD`, `c_RATIO`, `q_KNEE`).
- Sin cambios en sonido, presets o interfaz: actualización centrada en estabilidad.

### Removed
- Código legacy asociado a parámetros inexistentes en JCBTransient.

## 1.0.0
- Versión inicial.

# AzerothCore: Módulo Druid Forms Scales

Este módulo opcional para **AzerothCore (WotLK 3.3.5a)** permite a los jugadores ajustar el tamaño y escala de **cada una de las transformaciones del Druida de forma independiente**.

Los tamaños se pueden configurar en cualquier momento (incluso en forma racial) y se aplicarán automáticamente cuando el jugador adquiera la forma correspondiente. Diseñado para trabajar en perfecta armonía con `mod-character-scale` y `mod-druid-forms`.

## Características

* Configuración de escala persistente individual para cada transformación.
* No requiere estar en la forma específica para configurar su escala.
* Agrupación inteligente de auras (ej. `.bear_scale` escala tanto el Oso normal como el Temible, `.flight_scale` escala la forma Voladora normal y la Épica).
* Aplicación inmediata y automática tras *shapeshifting*, reloguear o cambiar de mapa.

## Comandos

* `.bear_scale <valor>` - Escala para Forma de Oso y Oso Temible.
* `.cat_scale <valor>` - Escala para Forma Felina.
* `.moonkin_scale <valor>` - Escala para Forma de Lechúcico Lunar.
* `.tree_scale <valor>` - Escala para Forma de Árbol de Vida.
* `.flight_scale <valor>` - Escala para Forma Voladora y Forma Voladora Presta.
* `.aquatic_scale <valor>` - Escala para Forma Acuática.
* `.travel_scale <valor>` - Escala para Forma de Viaje.

*(Nota: Para cambiar la escala de la forma Humanoide/Racial base, utiliza `.character_scale` provisto por su respectivo módulo).*

## Instalación

1. Descarga o clona este repositorio dentro de la carpeta `modules/` de tu código fuente de AzerothCore:

    ```bash
    cd modules/
    git clone [https://github.com/Leaf-Spring/mod-druid-forms-scales.git](https://github.com/Leaf-Spring/mod-druid-forms-scales.git)
    ```

2. Recompila el proyecto con CMake.

3. La tabla `character_druid_forms_scales` se creará automáticamente en la base de datos `characters` al iniciar `worldserver`.

4. (Opcional) Copia `conf/mod_druid_forms_scales.conf.dist` a la carpeta `configs/` de tu servidor como `mod_druid_forms_scales.conf` para ajustar los límites globales.

---

## Apoyo
Si este módulo te ha sido de utilidad y deseas apoyar el desarrollo de más herramientas de open-source, puedes invitarme un café:
☕ [buymeacoffee.com/foliaprintempo](https://buymeacoffee.com/foliaprintempo)
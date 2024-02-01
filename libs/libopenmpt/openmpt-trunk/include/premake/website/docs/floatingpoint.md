Specifies the style of floating point math which should be used.

```lua
floatingpoint "value"
```

If no value is set for a configuration, the toolset's default floating point settings will be used.

### Parameters ###

`value` specifies the desired style of floating point math:

| Value       | Description                                                       |
|-------------|-------------------------------------------------------------------|
| Default     | Use the toolset's floating point settings.                        |
| Fast        | Enable floating point optimizations at the expense of accuracy.   |
| Strict      | Improve floating point consistency at the expense of performance. |
| None        | Needs documentation                                               |

### Applies To ###

Project configurations.

### Availability ###

Premake 5.0 or later.

The **None** option is only available for VisualD projects.

### Examples ###

```lua
floatingpoint "Fast"
```

### See Also ###

* [vectorextensions](vectorextensions.md)

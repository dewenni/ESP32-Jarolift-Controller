# Beispiel Fernbedienungen und deren Zusammenspiel von Seriennummern und Bitmasken

## TDRC 01W

```
serial: 0x033d9c00 | cmd: UP, 	| channel: 00000000 00000001  
serial: 0x033d9c00 | cmd: DOWN, | channel: 00000000 00000001  
serial: 0x033d9c00 | cmd: STOP, | channel: 00000000 00000001  
```

## TDRC 02W

### Kanal 1:
```
serial: 0x04d46401 | cmd: UP, 	| channel: 00000000 00000010  
serial: 0x04d46401 | cmd: DOWN, | channel: 00000000 00000010  
serial: 0x04d46401 | cmd: STOP, | channel: 00000000 00000010  
```
### Kanal 2:
```
serial: 0x04d46501 | cmd: UP, 	| channel: 00000000 00000010  
serial: 0x04d46501 | cmd: DOWN, | channel: 00000000 00000010  
serial: 0x04d46501 | cmd: STOP, | channel: 00000000 00000010  
```

## TDRCT 04W

### Kanal 1:
```
serial: 0x0c990000 | cmd: UP, 	| channel: 00000000 00000001  
serial: 0x0c990000 | cmd: DOWN, | channel: 00000000 00000001  
serial: 0x0c990000 | cmd: STOP, | channel: 00000000 00000001  
```

### Kanal 2:
```
serial: 0x0c990001 | cmd: UP, 	| channel: 00000000 00000010  
serial: 0x0c990001 | cmd: DOWN, | channel: 00000000 00000010  
serial: 0x0c990001 | cmd: STOP, | channel: 00000000 00000010  
```

### Kanal 3:
```
serial: 0x0c990002 | cmd: UP, 	| channel: 00000000 00000100  
serial: 0x0c990002 | cmd: DOWN, | channel: 00000000 00000100  
serial: 0x0c990002 | cmd: STOP, | channel: 00000000 00000100  
```

### Kanal 4:
```
serial: 0x0c990003 | cmd: UP, 	| channel: 00000000 00001000  
serial: 0x0c990003 | cmd: DOWN, | channel: 00000000 00001000  
serial: 0x0c990003 | cmd: STOP, | channel: 00000000 00001000  
```

### Gruppe (alle)
```
serial: 0x0c990004 | cmd: UP, 	| channel: 00000000 00001111  
serial: 0x0c990004 | cmd: DOWN, | channel: 00000000 00001111  
serial: 0x0c990004 | cmd: STOP, | channel: 00000000 00001111  
```

## TDRC 16W

### Kanal 1:
```
serial: 0x004c0000 | cmd: UP, 	| channel: 00000000 00000001  
serial: 0x004c0000 | cmd: DOWN, | channel: 00000000 00000001  
serial: 0x004c0000 | cmd: STOP, | channel: 00000000 00000001  
```

### Kanal 4:
```
serial: 0x004c0003 | cmd: UP, 	| channel: 00000000 00001000  
serial: 0x004c0003 | cmd: DOWN, | channel: 00000000 00001000  
serial: 0x004c0003 | cmd: STOP, | channel: 00000000 00001000  
```

### Kanal 8:
```
serial: 0x004c0007 | cmd: UP, 	| channel: 00000000 10000000  
serial: 0x004c0007 | cmd: DOWN, | channel: 00000000 10000000  
serial: 0x004c0007 | cmd: STOP, | channel: 00000000 10000000  
```

### Kanal 16:
```
serial: 0x004c000f | cmd: UP, 	| channel: 10000000 00000000  
serial: 0x004c000f | cmd: DOWN, | channel: 10000000 00000000  
serial: 0x004c000f | cmd: STOP, | channel: 10000000 00000000  
```

### Gruppe aus Kanal 1, 4, 8, 12, 16
```
serial: 0x004c0000 | cmd: UP, 	| channel: 10001000 10001001  
serial: 0x004c0000 | cmd: DOWN, | channel: 10001000 10001001  
serial: 0x004c0000 | cmd: STOP, | channel: 10001000 10001001  
```

### Gruppe aus Kanal 1-6 + Kanalschalter steht auf 6
```
serial: 0x004c0005 | cmd: UP, 	| channel: 00000000 00111111  
serial: 0x004c0005 | cmd: DOWN, | channel: 00000000 00111111  
serial: 0x004c0005 | cmd: STOP, | channel: 00000000 00111111  
```

### Gruppe aus Kanal 1-6 + Kanalschalter steht auf 15
```
serial: 0x004c000e | cmd: UP,   | channel: 01000000 00111111  
serial: 0x004c000e | cmd: DOWN, | channel: 01000000 00111111  
serial: 0x004c000e | cmd: STOP, | channel: 01000000 00111111  
```

> [!NOTE]
> Die Seriennummer verändert sich dabei je nachdem welcher Einzel-Kanal ausgewählt ist, bevor man auf die Gruppenbedienung umschaltet - Bei 0x004c0005 war der Kanal 6 noch ausgewählt - Bei 0x004c000e war der Kanal 15 noch ausgewählt.

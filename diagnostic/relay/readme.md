
# Activate Python venv 
```
source ../venv/bin/activate
```


# Run all relay for 5 seconds each (continuous cycle)

```
python3 relay_switcher.py --continuous

# or 

python3 relay_switcher.py -c
```

# Run all relay for 5 seconds each (1 cycle)

```
python3 relay_switcher.py --single

# or

python3 relay_switcher.py
```


# Run single relay for 5 seconds

```
python3 relay_switcher.py --appliance Light

# or

python3 relay_switcher.py -a light
```


# list of valid appliance name(s):
- `filter`
- `co2`
- `light`
- `heater`
- `hangonfilter`
- `wavemaker`
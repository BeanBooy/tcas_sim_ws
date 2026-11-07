
class Plane:
    def __init__(self, icao):
        self.icao = icao
        self.color = None
        self.selected = False
        self.max_size = 200
        # array for data history
        self.lats = []
        self.lons = []
        self.alts = []
        self.times = []
        self.last_update_time = 0.0

    def add_position(self, lat, lon, alt, timestamp):
        self.lats.append(lat)
        self.lons.append(lon)
        self.alts.append(alt)
        self.times.append(timestamp)
        # cap array size
        if len(self.lats) > self.max_size:
            self.lats.pop(0)
            self.lons.pop(0)
            self.alts.pop(0)
            self.times.pop(0)
        # update last ping time
        self.last_update_time = timestamp

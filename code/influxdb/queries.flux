from(bucket: "WW")
  |> range(start: -1d)
  |> filter(fn: (r) => r._measurement == "telemetry")
  |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
  |> sort(columns: ["Время измерения"], desc: true)





from(bucket: "WW")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "telemetry")
  |> filter(fn: (r) => r._field == "water_temperature")
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
  |> yield(name: "raw_water_level")
  
  
from(bucket: "WW")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "telemetry")
  |> filter(fn: (r) => r._field == "water_temperature")
  |> timedMovingAverage(every: 10m, period: 2h)
  |> yield(name: "moving_average_2h")
  
  
  
  from(bucket: "WW")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "telemetry")
  |> filter(fn: (r) => r._field == "water_temperature")
  |> timedMovingAverage(every: 10m, period: 2h)
  |> yield(name: "moving_average_2h")






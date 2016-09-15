% ThingSpeak Weather Station Data Analysis
%
% This script includes examples of reading data from ThingSpeak channel and
% performing various kind of visualization on the data. 
%
% Retrieve the data first (see the first section) before executing other sections. 
%
% ThingSpeak Support Toolbox on File Exchange is required to run this script.
% It can be downloaded here:
% http://www.mathworks.com/matlabcentral/fileexchange/52244-thingspeak-support-toolbox
%
% Copyright 2016 The MathWorks, Inc.


%% Retrieve data from ThingSpeak channel

% Channel ID to read data from
readChannelID = 60917;

% Read data including the timestamp, and channel information for the last week
[data,time,channelInfo] = thingSpeakRead(readChannelID,'Fields',1:7,'NumDays',7);                    
% Create variables to store different sorts of data
temperatureData = data(:,1);
pressureData = data(:,2);
humidityData = data(:,3);
voltageData = data(:,4);
rainData = data(:,7);
windSpeedData = data(:,5);
windGustData = data(:,7);
windDirectionData = data(:,6);

%% Temperature, Humidity, Pressure, Rain, WindSpeed, WindDirection histogram

% Create a figure to display plots
figure

% Temperature histogram
subplot(2,3,1) % Create 2-by-3 axis on the same figure, and work on the first axis
histogram(temperatureData);
title(channelInfo.FieldDescriptions{1});
grid on

% Humidity histogram
subplot(2,3,2)
histogram(humidityData);
title(channelInfo.FieldDescriptions{2});
grid on

% Pressure histogram
subplot(2,3,3)
histogram(pressureData);
title(channelInfo.FieldDescriptions{3});
grid on

% Rain fall histogram
subplot(2,3,4)
histogram(rainData);
title(channelInfo.FieldDescriptions{4});
grid on

% WindSpeed histogram
subplot(2,3,5)
histogram(windSpeedData);
title(channelInfo.FieldDescriptions{5});
grid on

% Voltage histogram
subplot(2,3,6)
histogram(voltageData);
title(channelInfo.FieldDescriptions{4});
grid on

figure 
% Wind Direction histogram
rad = windDirectionData*pi/180; % Convert to radians
rad = -rad+pi/2; % Adjust the wind direction data to match map compass, such that North is equal to 0 degree
%subplot(2,3,6)
rose(rad,12) % Plot the wind direction histogram in a polar axis 
title(channelInfo.FieldDescriptions{7})
ax = gca;
ax.View = [-90 90]; % Rotate axis 90 degrees counterclock-wise such that North is equal to 0 degree

%% Smooth Temperature and Trend

% Remove missing data from the temperature variable, in order to perform
% the fitting methods
idx = ~isnan(temperatureData);
rawTemp = temperatureData(idx);
newTime = time(idx);

% Smooth the raw temperature data with local 60-point mean values
smoothTemp = movmean(temperatureData(idx),60);

% Fit the data for a trend line
[p,~,mu]= polyfit(datenum(newTime),rawTemp,1);
trend = polyval(p,datenum(newTime),[],mu);

% Optional: fit the data by using the sum of eight sine functions
% To use the fit function, the Curve Fitting Toolbox is required.
% It can be downloaded here: http://www.mathworks.com/products/curvefitting/
%f = fit(datenum(newTime),rawTemp,'sin8');

% Plot the raw data, smooth data, trend and fitting curve
figure
hold on
plot(newTime,rawTemp,'b') % Plot the raw temperature 
plot(newTime,smoothTemp,'g','LineWidth',1.5) % Plot the smoothing curve
plot(newTime,trend,'m','LineWidth',2) % Plot the trend line
plot(f) % Plot the fitting curve
hold off
%xlim([datenum(time(1)) datenum(time(end))]) % Adjust the x-axis to properly display the curves
xlabel('Date')
ylabel('Temperature (C)')
legend({'Raw Data','Smooth Data','Trend'},'Location','NE')


%% Daily Temperature Statistics

% Remove missing data
idx = ~isnan(temperatureData);
rawTemp = temperatureData(idx);
newTime = time(idx);

% Plot temperature for each day. 
figure
hold on
% Use dateshift and findgroups functions to bin the temperature data in
% terms of individual day
timeShift = dateshift(newTime,'start','day'); % Shift hour, minute, and second to the start of each day, i.e. 00:00:00
dayGroup = findgroups(timeShift); % Group the shifted time in terms of day
splitapply(@plot,second(newTime,'secondofday')/3600,rawTemp,dayGroup); % Apply the plot function to the temperature for each day by using the group defined above

% Compute the absolute Max, Min, and Mean for each hour 
hourGroup = findgroups(hour(newTime)); % Create a vector of grouping number for each hour
tMax = splitapply(@max,rawTemp,hourGroup); % Calculate the max for each hour
tMin = splitapply(@min,rawTemp,hourGroup); % Calculate the min for each hour
tMean = splitapply(@mean,rawTemp,hourGroup); % Calculate the mean for each hour

% Prepare x and y data for the average and the variation area
xHalf = [0,repelem(1:23,2),24]; % x coordinate at the end points for each interval (each hour)
x = [xHalf, fliplr(xHalf)]; % x coordinate for variation area (including both bottom and top profiles)
tMax = repelem(tMax,2); % y coordinate for max per hour
tMin = repelem(tMin,2); % y coordinate for min per hour
tMean = repelem(tMean,2);% y coordinate for mean per hour
y = [tMin; flipud(tMax)]; % y coordinate for variation area (bottom and top)

% Plot Statistics
plot([0,24], [max(tMax),max(tMax)],'.--k', 'LineWidth',1.5) % Plot global Max as a horizontal line
plot([0,24], [min(tMin),min(tMin)],'.--k', 'LineWidth',1.5) % Plot global Min as a horizontal line
h1 = fill(x,y, 'b', 'LineStyle', 'none', 'FaceAlpha', 0.1); % Highlight the variation area per hour
h2 = plot(xHalf,tMean,'--','LineWidth',2); % Plot average per hour
hold off
title('Daily temperature over the past week')
ylabel('Temperature (C)')
xlabel('Day Time')
axis tight % Adjust axis to fit the plot
ylim([20 80]) % Adjust to allow space on the top and at the bottom of the axis
ax = gca; % Get the current axis object
ax.XTick = 1:24; % Change the X-Tick to 24 hours 
legend([h1,h2],'Variation per hour','Average per hour')


%% Temperature and Humidity 3D bar charts

% Create a day range vector
dayRange = day(time(1):time(end));
% Pre-allocate matrix
weatherData = zeros(length(dayRange),24);

% Generate temperature 3D bar chart
% Get temperature per whole clock for each day
for m = 1:length(dayRange) % Loop over all days
    for n = 1:24 % Loop over 24 hours
        if any(day(time)==dayRange(m) & hour(time)==n); % Check if data exist for this specific time
            hourlyData = temperatureData((day(time)==dayRange(m) & hour(time)==n)); % Pull out the hourly temperature from the matrix
            weatherData(m,n) = hourlyData(1); % Assign the temperature at the time closest to the whole clock
        end
    end
end

% Plot
figure
h = bar3(datenum(time(1):time(end)), weatherData);
for k = 1:length(h) % Change the face color for each bar
    h(k).CData = h(k).ZData;
    h(k).FaceColor = 'interp';
end
title('Temperature Distribution')
xlabel('Hour of Day')
ylabel('Date')
datetick('y','mmm dd') % Change the Y-Tick to display specified date format 
ax = gca;
ax.XTick = 1:24; % Change the X-Tick to 24 hours 
ax.YTickLabelRotation = 30; % Rotate label for better display
colorbar % Add a color bar to indicate the scaling of color


% Generate humidity 3D bar chart
% Get humidity per whole clock for each day
for m = 1:length(dayRange) % Loop over all days
    for n = 1:24 % Loop over 24 hours
        if any(day(time)==dayRange(m) & hour(time)==n); % Check if data exist for this specific time
            hourlyData = humidityData((day(time)==dayRange(m) & hour(time)==n)); % Pull out the hourly humidity from the matrix
            weatherData(m,n) = hourlyData(1); % Assign the humidity at the time closest to the whole clock
        end
    end
end

% Plot
figure
h = bar3(datenum(time(1):time(end)), weatherData);
for k = 1:length(h) % Change the face color for each bar
    h(k).CData = h(k).ZData;
    h(k).FaceColor = 'interp';
end
title('Humidity Distribution')
xlabel('Hour of Day')
ylabel('Date')
datetick('y','mmm dd') % Change the Y-Tick to display specified date format 
ax = gca;
ax.XTick = 1:24; % Change the X-Tick to 24 hours 
ax.YTickLabelRotation = 30; % Rotate label for better display
colorbar % Add a color bar to indicate the scaling of color


%% Dew Point

% Convert temperature from Fahrenheit to Celsius
tempC = (5/9)*(temperatureData-32);

% Calculate dew point. Refer to Wiki (https://en.wikipedia.org/wiki/Dew_point) for the formula and more details  
% Specify the constants for water vapor (b) and barometric (c) pressure.
b = 17.67;
c = 243.5;
% Calculate the intermediate value 'gamma'
gamma = log(humidityData/100) + b*tempC ./ (c+tempC);
% Calculate dew point in Celsius
dewPoint = c*gamma ./ (b-gamma);

% Convert to dew point in Fahrenheit
dewPointF = (dewPoint*1.8) + 32;

% Plot
figure
hold on
plot(time, dewPointF,'d') % Plot dew points
xlabel('Time')
ylabel('Dew Point')
%xlim([datenum(time(1)) datenum(time(end))]) % Adjust the x-axis limits
fill([xlim fliplr(xlim)], [68 68 80 80], 'r', 'LineStyle', 'none', 'FaceAlpha', 0.1) % Highlight the uncomfortable zone
text(0.7*datenum(time(1)) + 0.3*datenum(time(end)), 75, 'Uncomfortable', 'FontWeight','bold') % Add text for the zone
fill([xlim fliplr(xlim)], [50 50 68 68], 'g', 'LineStyle', 'none', 'FaceAlpha', 0.1) % Highlight the comfortable zone
text(0.7*datenum(time(1)) + 0.3*datenum(time(end)), 60, 'Comfortable', 'FontWeight','bold') % Add text for the zone
fill([xlim fliplr(xlim)], [min(ylim) min(ylim) 50 50], 'y', 'LineStyle', 'none', 'FaceAlpha', 0.1) % Highlight the dry zone
text(0.65*datenum(time(1)) + 0.35*datenum(time(end)), 20, 'Dry', 'FontWeight','bold') % Add text for the zone
hold off


%% Wind Compass and Feather

% Specify the latest n+1 wind directions to be displayed
n = 9;

% Convert to radians
rad = windDirectionData*pi/180;

% Create a feather plot
% Remove missing data and any wind speed with value 0 
idx = (~isnan(rad)) & (~isnan(windSpeedData)) & (windSpeedData~=0);
% Convert polar coordinates to Cartesian. Note that dividing by the maximum
% wind speed allows to scale the length of each arrow by its relative wind
% speed, rather than the wind direction.
[x,y] = pol2cart(rad(idx),windSpeedData(idx)/max(windSpeedData(idx)));
% Plot
figure
subplot(2,1,2)
feather(x((end-n):end),y((end-n):end)) % Plot the feather
xlim([0 n+2]) % Adjust the x-axis
ylim([-1 1]) % Adjust the y-axis
xlabel(['The last ',num2str(n+1),' wind direction']) % Add x lable
title('Wind Direction Changes')
grid on
ax = gca;
ax.YTickLabel = {}; % Hide the Y-Tick
ax.XTick = 1:(n+1); % Adjust the X-Tick for n+1 data

% Create a compass plot
% Adjust the wind direction to match map compass, such that North is equal to 0 degree
rad = -rad+pi/2;
% Calculate the cosine component
u = cos(rad) .* windSpeedData; % x coordinate of wind speed on circular plot
% Calculate the sine component
v = sin(rad) .* windSpeedData; % y coordinate of wind speed on circular plot
% Plot
subplot(2,1,1)
compass(u((end-n):end),v((end-n):end)) % Plot compass
title('Wind Compass')
ax = gca;
ax.View = [-90 90]; % Rotate axis 90 degrees counterclock-wise such that North is equal to 0 degree


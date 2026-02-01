graphics_toolkit("gnuplot");
output_img = 'route_comparison.png';

% 1. Read Coordinates from berlin52.tsp
% (Skipping header manually to be robust)
fid = fopen('berlin52.tsp', 'r');
coords = [];
reading = 0;
while ~feof(fid)
    line = fgetl(fid);
    if strncmp(line, 'NODE_COORD_SECTION', 18), reading = 1; continue; end
    if strncmp(line, 'EOF', 3), break; end
    if reading
        c = sscanf(line, '%d %f %f');
        if length(c) == 3, coords = [coords; c(2) c(3)]; end
    end
end
fclose(fid);

% 2. Read My Route
if exist('my_route.txt', 'file')
    my_route = load('my_route.txt'); 
    my_route = my_route + 1; % Convert 0-based (C) to 1-based (Matlab)
    my_route = [my_route; my_route(1)]; % Close the loop
else
    error("my_route.txt missing. Run the simulation first.");
end

% 3. Read Optimal Route
opt_route = [];
if exist('berlin52.opt.tour', 'file')
    fid = fopen('berlin52.opt.tour', 'r');
    while ~feof(fid)
        line = fgetl(fid);
        if strcmp(line, 'TOUR_SECTION'), break; end
    end
    opt_route = fscanf(fid, '%d');
    opt_route = opt_route(opt_route ~= -1);
    opt_route = [opt_route; opt_route(1)];
    fclose(fid);
end

% --- PLOT ---
h = figure('visible', 'off', 'Position', [0,0,1000,800]);

% Plot Cities
plot(coords(:,1), coords(:,2), 'ko', 'MarkerFaceColor', 'k', 'MarkerSize', 6); hold on;

% Plot Optimal (Red Dashed)
if ~isempty(opt_route)
    plot(coords(opt_route,1), coords(opt_route,2), 'r--', 'LineWidth', 2);
end

% Plot Mine (Blue Solid)
% Shift slightly so it doesn't perfectly hide the red line if identical
plot(coords(my_route,1)+1, coords(my_route,2)+1, 'b-', 'LineWidth', 1.5);

legend({'Cities', 'Optimal Tour (Red)', 'My Solution (Blue)'}, 'Location', 'northeast');
title('Route Comparison: Berlin52', 'FontSize', 16, 'FontWeight', 'bold');
xlabel('X Coordinate'); ylabel('Y Coordinate');
grid on;

print(output_img, '-dpng');

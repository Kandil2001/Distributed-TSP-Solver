graphics_toolkit("gnuplot");
output_img = 'route_comparison_tsplib_euc2d.png';

% 1. Read coordinates from the included TSPLIB instance.
fid = fopen('data/berlin52.tsp', 'r');
if fid < 0, error('Could not open data/berlin52.tsp'); end
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

% 2. Read the solver route, written as zero-based city indices.
if exist('my_route.txt', 'file')
    my_route = load('my_route.txt') + 1;
    my_route = [my_route; my_route(1)];
else
    error('my_route.txt is missing. Run the solver first.');
end

% 3. Read the official one-based optimal tour.
fid = fopen('data/berlin52.opt.tour', 'r');
if fid < 0, error('Could not open data/berlin52.opt.tour'); end
while ~feof(fid)
    line = fgetl(fid);
    if strcmp(strtrim(line), 'TOUR_SECTION'), break; end
end
opt_route = fscanf(fid, '%d');
opt_route = opt_route(opt_route ~= -1);
opt_route = [opt_route; opt_route(1)];
fclose(fid);

h = figure('visible', 'off', 'Position', [0,0,1000,800]);
plot(coords(:,1), coords(:,2), 'ko', 'MarkerFaceColor', 'k', 'MarkerSize', 6); hold on;
plot(coords(opt_route,1), coords(opt_route,2), 'r--', 'LineWidth', 2);
plot(coords(my_route,1)+1, coords(my_route,2)+1, 'b-', 'LineWidth', 1.5);
legend({'Cities', 'Official optimal tour', 'Solver route'}, 'Location', 'northeast');
title('Berlin52 Route Comparison: TSPLIB EUC\_2D', 'FontSize', 16, 'FontWeight', 'bold');
xlabel('X Coordinate'); ylabel('Y Coordinate');
grid on;
print(output_img, '-dpng');

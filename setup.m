run();

function [] = run()

    connection = tcpclient('127.0.0.1', 8080);
    
    while true
        if connection.BytesAvailable > 0
            nPlots = read(connection, 1, "uint32");
            break
        end
    end
    
    plots = [];
    figures = [];
    
    for index = 1:nPlots
        f = figure();
        p = plot(0, 0);
        plots = [p, plots];
        figures = [f, figures];
    end
    
    maximumNPoints = 3000;
    xData = [];
    yData = [];

    cleanupObject = onCleanup(@() runCleanup(connection, figures));
    
    while true
        if connection.BytesAvailable > 0
            
            nFloatsInSegment = nPlots * 2;
            nFloats = floor(connection.BytesAvailable / 4);
            received = read(connection, floor(nFloats / nFloatsInSegment) * nFloatsInSegment, "single");
            
            receivedXs = received(1:2:end);
            xData = [xData; reshape(receivedXs, [nPlots, floor(length(receivedXs) / nPlots)])'];
            xData = xData(max(end - maximumNPoints + 1, 1):end, :);
            
            receivedYs = received(2:2:end);
            yData = [yData; reshape(receivedYs, [nPlots, floor(length(receivedYs) / nPlots)])'];
            yData = yData(max(end - maximumNPoints + 1, 1):end, :);

            for index = 1:size(xData, 2)
                set(plots(index), 'XData', xData(:, index), 'YData', yData(:, index));
                refreshdata;
                drawnow;
            end
        end
    end
end

function [] = runCleanup(connection, figures)
    try
        delete(connection);
    catch
    end
    for index = 1:length(figures)
        try
            close(figures(index));
        catch
        end
    end
end
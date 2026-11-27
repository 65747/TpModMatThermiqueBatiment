#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct Material {
    std::string name;
    double conductivityWPerM_K;
};

struct Layer {
    Material material;
    double thicknessM;
};

struct Wall {
    double areaM2;
    std::vector<Layer> layers;

    double resistanceM2_KPerW() const {
        double resistance = 0.0;
        for (const Layer& layer : layers) {
            if (layer.material.conductivityWPerM_K <= 0.0 || layer.thicknessM <= 0.0) {
                throw std::invalid_argument("La conductivité et l'épaisseur doivent être positives.");
            }
            resistance += layer.thicknessM / layer.material.conductivityWPerM_K;
        }
        return resistance;
    }

    double heatLossW(double insideC, double outsideC) const {
        if (areaM2 <= 0.0) {
            throw std::invalid_argument("La surface du mur doit être positive.");
        }
        const double resistance = resistanceM2_KPerW();
        if (resistance <= 0.0) {
            throw std::invalid_argument("Le mur doit contenir au moins une couche.");
        }
        return (insideC - outsideC) * areaM2 / resistance;
    }
};

struct Building {
    std::vector<Wall> walls;

    double heatLossCoefficientWPerK() const {
        double coefficient = 0.0;
        for (const Wall& wall : walls) {
            const double resistance = wall.resistanceM2_KPerW();
            if (wall.areaM2 <= 0.0 || resistance <= 0.0) {
                throw std::invalid_argument("La surface et la résistance du mur doivent être positives.");
            }
            coefficient += wall.areaM2 / resistance;
        }
        return coefficient;
    }

    double heatLossW(double insideC, double outsideC) const {
        return heatLossCoefficientWPerK() * (insideC - outsideC);
    }
};

struct WeatherSample {
    double timeHours;
    double outsideC;
};

struct Room {
    double capacityJPerK;
    double conductanceWPerK;

    double timeConstantSeconds() const {
        return capacityJPerK / conductanceWPerK;
    }

    double temperatureExplicit(double currentC, double outsideC, double stepSeconds) const {
        return currentC + stepSeconds * conductanceWPerK / capacityJPerK
            * (outsideC - currentC);
    }

    double temperatureImplicit(double currentC, double outsideC, double stepSeconds) const {
        const double factor = stepSeconds * conductanceWPerK / capacityJPerK;
        return (currentC + factor * outsideC) / (1.0 + factor);
    }
};

std::vector<WeatherSample> loadWeather(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Impossible d'ouvrir le fichier météo : " + path);
    }

    std::vector<WeatherSample> samples;
    std::string line;
    std::getline(input, line);
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const std::size_t separator = line.find(';');
        if (separator == std::string::npos || separator + 1 >= line.size()) {
            throw std::runtime_error("Une ligne du fichier météo est incorrecte.");
        }
        const double outsideC = std::stod(line.substr(separator + 1));
        if (!std::isfinite(outsideC)) {
            throw std::runtime_error("Le fichier météo contient une température incorrecte.");
        }
        samples.push_back({static_cast<double>(samples.size()), outsideC});
    }
    if (samples.size() < 2) {
        throw std::runtime_error("Le fichier météo doit contenir au moins deux températures horaires.");
    }
    return samples;
}

double outsideTemperatureAt(const std::vector<WeatherSample>& weather, double timeHours) {
    const double lastHour = weather.back().timeHours;
    if (timeHours >= lastHour) {
        return weather.back().outsideC;
    }
    const std::size_t left = static_cast<std::size_t>(timeHours);
    const double fraction = timeHours - static_cast<double>(left);
    return weather[left].outsideC
        + fraction * (weather[left + 1].outsideC - weather[left].outsideC);
}

Building witnessBuilding() {
    const Material concrete{"Béton", 2.0};
    const Wall wall{50.0, {{concrete, 0.50}}};
    return Building{{wall}};
}

void runStaticExample() {
    const Building building = witnessBuilding();
    std::cout << std::fixed << std::setprecision(0);
    std::cout << "Pertes pour le mur témoin (50 m2 de béton, épaisseur 0,50 m)\n";
    std::cout << "À 20 °C dedans et 0 °C dehors : "
              << building.heatLossW(20.0, 0.0) << " W\n";
    std::cout << "À 20 °C dedans et -20 °C dehors : "
              << building.heatLossW(20.0, -20.0) << " W\n";
}

void runDynamic(const std::vector<WeatherSample>& weather, double initialInsideC,
                double stepSeconds, bool implicit) {
    if (!std::isfinite(initialInsideC) || !std::isfinite(stepSeconds) || stepSeconds <= 0.0) {
        throw std::invalid_argument("La température initiale doit être finie et le pas positif.");
    }

    const Building building = witnessBuilding();
    const Room room{460440.8, building.heatLossCoefficientWPerK()};
    const double timeConstant = room.timeConstantSeconds();
    if (!implicit && stepSeconds > 2.0 * timeConstant) {
        std::cerr << "Attention : le pas choisi peut rendre Euler explicite instable.\n";
    } else if (!implicit && stepSeconds > timeConstant) {
        std::cerr << "Attention : la température peut osciller avec Euler explicite.\n";
    }

    double insideC = initialInsideC;
    double minimumC = insideC;
    double maximumC = insideC;
    const double lastTimeSeconds = (weather.back().timeHours + 1.0) * 3600.0;

    std::cout << "temps_s;T_ext_C;T_int_C\n";
    std::cout << std::fixed << std::setprecision(3);
    for (double timeSeconds = 0.0; timeSeconds <= lastTimeSeconds; timeSeconds += stepSeconds) {
        const double outsideC = outsideTemperatureAt(weather,
            std::min(timeSeconds / 3600.0, weather.back().timeHours));
        std::cout << timeSeconds << ';' << outsideC << ';' << insideC << '\n';
        minimumC = std::min(minimumC, insideC);
        maximumC = std::max(maximumC, insideC);
        if (timeSeconds + stepSeconds > lastTimeSeconds) {
            break;
        }
        insideC = implicit
            ? room.temperatureImplicit(insideC, outsideC, stepSeconds)
            : room.temperatureExplicit(insideC, outsideC, stepSeconds);
    }

    std::cerr << std::fixed << std::setprecision(2)
              << "C;" << room.capacityJPerK << " J/K\n"
              << "G;" << room.conductanceWPerK << " W/K\n"
              << "tau;" << timeConstant << " s\n"
              << "T_min;" << minimumC << " °C\n"
              << "T_max;" << maximumC << " °C\n";
}

int main(int argc, char** argv) {
    try {
        if (argc == 1) {
            runStaticExample();
            return 0;
        }

        if (argc == 6 && std::string(argv[1]) == "dynamic") {
            const std::string method = argv[5];
            if (method != "explicit" && method != "implicit") {
                throw std::invalid_argument("Choisir explicit ou implicit.");
            }
            runDynamic(loadWeather(argv[2]), std::stod(argv[3]),
                       std::stod(argv[4]), method == "implicit");
            return 0;
        }

        std::cerr << "Utilisation :\n"
                  << "  " << argv[0] << "\n"
                  << "  " << argv[0]
                  << " dynamic fichier.csv T_initiale_C pas_s explicit|implicit\n";
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "Erreur : " << error.what() << '\n';
        return 1;
    }
}

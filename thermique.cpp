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

    double heatLossW(double insideC, double outsideC) const {
        double totalLossW = 0.0;
        for (const Wall& wall : walls) {
            totalLossW += wall.heatLossW(insideC, outsideC);
        }
        return totalLossW;
    }
};

int main() {
    const Material concrete{"Béton", 2.0};
    const Wall wall{50.0, {{concrete, 0.50}}};
    const Building building{{wall}};

    std::cout << std::fixed << std::setprecision(0);
    std::cout << "Pertes pour le mur témoin (50 m2 de béton, épaisseur 0,50 m)\n";
    std::cout << "À 20 °C dedans et 0 °C dehors : "
              << building.heatLossW(20.0, 0.0) << " W\n";
    std::cout << "À 20 °C dedans et -20 °C dehors : "
              << building.heatLossW(20.0, -20.0) << " W\n";
    return 0;
}

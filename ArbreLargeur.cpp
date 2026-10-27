#include <iostream>
#include <queue>
#include <vector>

struct Node {
    int val;
    Node* left;
    Node* right;

    Node(int v) : val(v), left(nullptr), right(nullptr) {}
};

// Insère un nœud à la première position disponible en largeur
Node* insererDynamique(Node* racine, int val) {
    if (!racine) return new Node(val);

    std::queue<Node*> q;
    q.push(racine);

    while (!q.empty()) {
        Node* courant = q.front();
        q.pop();

        if (!courant->left) {
            courant->left = new Node(val);
            return racine;
        } else {
            q.push(courant->left);
        }

        if (!courant->right) {
            courant->right = new Node(val);
            return racine;
        } else {
            q.push(courant->right);
        }
    }
    return racine;
}

// Fonction récursive pour libérer dynamiquement tout l'arbre
void libererArbre(Node* racine) {
    if (!racine) return;
    libererArbre(racine->left);
    libererArbre(racine->right);
    delete racine;
}

void parcoursLargeur(Node* racine) {
    if (!racine) return;

    std::queue<Node*> file;
    file.push(racine);

    while (!file.empty()) {
        Node* courant = file.front();
        file.pop();

        std::cout << courant->val << " ";

        if (courant->left)  file.push(courant->left);
        if (courant->right) file.push(courant->right);
    }
    std::cout << '\n';
}

int main() {
    Node* racine = nullptr;
    std::vector<int> donnees = {1, 2, 3, 4, 5, 6};

    // Construction dynamique de l'arbre
    for (int v : donnees) {
        racine = insererDynamique(racine, v);
    }

    std::cout << "Parcours en largeur : ";
    parcoursLargeur(racine); // Affiche : 1 2 3 4 5 6

    libererArbre(racine);
    return 0;
}
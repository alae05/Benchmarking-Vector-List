// Benchmark std::vector vs std::list : recherche, insertions, suppressions
// Compilation : g++ -O2 -std=c++17 bench_list_vector.cpp -o bench
// Utilisation : ./bench mots.txt [nb_operations=1000]

#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <chrono>
#include <iterator>
#include <iomanip>

using namespace std;
using Clock = chrono::steady_clock;

static double ms(Clock::time_point a, Clock::time_point b) {
    return chrono::duration<double, milli>(b - a).count();
}

static void entete(const string& titre) {
    cout << "\n=== " << titre << " ===\n";
    cout << left << setw(36) << "Operation" << setw(10) << "Conteneur"
         << right << setw(17) << "Temps total" << setw(24) << "Temps moyen" << "\n";
    cout << string(87, '-') << endl;
}

static void ligne(const string& op, const string& cont, double t, size_t k) {
    cout << left << setw(36) << op << setw(10) << cont
         << right << setw(14) << fixed << setprecision(3) << t << " ms"
         << setw(14) << setprecision(6) << (t / k) << " ms/op" << endl;
}

// Barriere pour empecher le compilateur d'optimiser (ou de sortir de la boucle)
// une recherche dont le resultat est toujours le meme.
#if defined(_MSC_VER)
  #include <intrin.h>
  #define BARRIERE() _ReadWriteBarrier()
#else
  #define BARRIERE() asm volatile("" ::: "memory")
#endif
static volatile size_t puits = 0;

// Cherche R fois le mot w avec std::find ; retourne le temps total (ms).
template <class C>
static double chercher(const C& c, const string& w, size_t R, size_t& trouves) {
    trouves = 0;
    auto t0 = Clock::now();
    for (size_t i = 0; i < R; ++i) {
        BARRIERE();
        bool ok = (find(c.begin(), c.end(), w) != c.end());
        puits = puits + (ok ? 1 : 0);
        trouves += ok ? 1 : 0;
    }
    auto t1 = Clock::now();
    return ms(t0, t1);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage : " << argv[0] << " fichier.txt [nb_operations]\n";
        return 1;
    }
    const size_t K = (argc >= 3) ? stoull(argv[2]) : 1000;

    // ---------- 1. Chargement du fichier (mot par mot) ----------
    ifstream f(argv[1]);
    if (!f) { cerr << "Impossible d'ouvrir " << argv[1] << "\n"; return 1; }

    vector<string> v;
    list<string> l;
    string mot;

    auto t0 = Clock::now();
    while (f >> mot) v.push_back(mot);
    auto t1 = Clock::now();
    double tv = ms(t0, t1);

    t0 = Clock::now();
    for (const auto& m : v) l.push_back(m);
    t1 = Clock::now();
    double tl = ms(t0, t1);

    const size_t N = v.size();
    cout << "Nombre de mots charges : " << N
         << " | operations par test : " << K << endl;
    if (N == 0) return 0;

    entete("CHARGEMENT");
    ligne("Chargement (push_back)", "vector", tv, N);
    ligne("Chargement (push_back)", "list", tl, N);

    // reserve : evite que les reallocations faussent la comparaison
    v.reserve(N + 3 * K + 10);
    const string nouveau = "nouveau_mot";

    // =====================================================================
    //                             RECHERCHE
    // =====================================================================
    // std::find renvoie la 1ere occurrence. Dans un texte, un mot se repete :
    // on choisit donc des mots dont la PREMIERE occurrence est au debut,
    // vers le milieu, vers la fin, et un mot absent (parcours complet).
    string mDebut = v.front(), mMilieu, mFin;
    size_t pDebut = 0, pMilieu = 0, pFin = 0;
    {
        unordered_map<string, size_t> premier;
        premier.reserve(N);
        for (size_t i = 0; i < N; ++i) premier.emplace(v[i], i); // garde la 1ere position

        size_t ecart = N;
        for (const auto& kv : premier) {
            size_t p = kv.second;
            size_t e = (p > N / 2) ? p - N / 2 : N / 2 - p;
            if (e < ecart) { ecart = e; mMilieu = kv.first; pMilieu = p; }
            if (p >= pFin) { pFin = p; mFin = kv.first; }
        }
    }
    const string mAbsent = "mot_absent_introuvable_xyz";
    const size_t R = min<size_t>(K, 100);   // une recherche = O(n), on limite
    size_t trouves = 0;

    cout << "\nMots recherches (position de la 1ere occurrence) :\n"
         << "  debut   : \"" << mDebut  << "\" -> " << pDebut  << "\n"
         << "  milieu  : \"" << mMilieu << "\" -> " << pMilieu << "\n"
         << "  fin     : \"" << mFin    << "\" -> " << pFin    << "\n"
         << "  absent  : \"" << mAbsent << "\" -> parcours complet\n"
         << "Recherches par test : " << R << endl;

    entete("RECHERCHE (std::find)");
    struct Cas { const char* nom; const string* w; };
    Cas cas[] = { {"Recherche mot au debut",  &mDebut},
                  {"Recherche mot au milieu", &mMilieu},
                  {"Recherche mot vers la fin", &mFin},
                  {"Recherche mot absent",    &mAbsent} };
    for (const auto& c : cas) {
        double t = chercher(v, *c.w, R, trouves);
        ligne(c.nom, "vector", t, R);
        t = chercher(l, *c.w, R, trouves);
        ligne(c.nom, "list", t, R);
    }

    // =====================================================================
    //                            INSERTIONS
    // =====================================================================
    entete("INSERTIONS");

    // --- Tete ---
    t0 = Clock::now();
    for (size_t i = 0; i < K; ++i) v.insert(v.begin(), nouveau);
    t1 = Clock::now();
    ligne("Insertion en tete", "vector", ms(t0, t1), K);

    t0 = Clock::now();
    for (size_t i = 0; i < K; ++i) l.push_front(nouveau);
    t1 = Clock::now();
    ligne("Insertion en tete", "list", ms(t0, t1), K);

    // --- Queue ---
    t0 = Clock::now();
    for (size_t i = 0; i < K; ++i) v.push_back(nouveau);
    t1 = Clock::now();
    ligne("Insertion en queue", "vector", ms(t0, t1), K);

    t0 = Clock::now();
    for (size_t i = 0; i < K; ++i) l.push_back(nouveau);
    t1 = Clock::now();
    ligne("Insertion en queue", "list", ms(t0, t1), K);

    // --- Milieu ---
    t0 = Clock::now();
    for (size_t i = 0; i < K; ++i) v.insert(v.begin() + v.size() / 2, nouveau);
    t1 = Clock::now();
    ligne("Insertion au milieu", "vector", ms(t0, t1), K);

    t0 = Clock::now();
    for (size_t i = 0; i < K; ++i) {
        auto it = l.begin();
        advance(it, l.size() / 2);          // parcours O(n) a chaque fois
        l.insert(it, nouveau);
    }
    t1 = Clock::now();
    ligne("Insertion milieu (avec parcours)", "list", ms(t0, t1), K);

    {
        auto mid = l.begin();
        advance(mid, l.size() / 2);          // positionne une seule fois
        t0 = Clock::now();
        for (size_t i = 0; i < K; ++i) l.insert(mid, nouveau);
        t1 = Clock::now();
        ligne("Insertion milieu (iter. fixe)", "list", ms(t0, t1), K);
    }

    // =====================================================================
    //                            SUPPRESSIONS
    // =====================================================================
    entete("SUPPRESSIONS");

    // --- Tete ---
    t0 = Clock::now();
    for (size_t i = 0; i < K; ++i) v.erase(v.begin());
    t1 = Clock::now();
    ligne("Suppression en tete", "vector", ms(t0, t1), K);

    t0 = Clock::now();
    for (size_t i = 0; i < K; ++i) l.pop_front();
    t1 = Clock::now();
    ligne("Suppression en tete", "list", ms(t0, t1), K);

    // --- Queue ---
    t0 = Clock::now();
    for (size_t i = 0; i < K; ++i) v.pop_back();
    t1 = Clock::now();
    ligne("Suppression en queue", "vector", ms(t0, t1), K);

    t0 = Clock::now();
    for (size_t i = 0; i < K; ++i) l.pop_back();
    t1 = Clock::now();
    ligne("Suppression en queue", "list", ms(t0, t1), K);

    // --- Milieu ---
    t0 = Clock::now();
    for (size_t i = 0; i < K; ++i) v.erase(v.begin() + v.size() / 2);
    t1 = Clock::now();
    ligne("Suppression au milieu", "vector", ms(t0, t1), K);

    t0 = Clock::now();
    for (size_t i = 0; i < K; ++i) {
        auto it = l.begin();
        advance(it, l.size() / 2);          // parcours O(n) a chaque fois
        l.erase(it);
    }
    t1 = Clock::now();
    ligne("Suppression milieu (avec parcours)", "list", ms(t0, t1), K);

    {
        auto mid = l.begin();
        advance(mid, l.size() / 2);          // positionne une seule fois
        t0 = Clock::now();
        for (size_t i = 0; i < K; ++i) mid = l.erase(mid);  // erase renvoie le suivant
        t1 = Clock::now();
        ligne("Suppression milieu (iter. fixe)", "list", ms(t0, t1), K);
    }

    cout << "\nTailles finales : vector = " << v.size()
         << ", list = " << l.size()
         << " (recherches trouvees au dernier test : " << trouves << ")" << endl;
    return 0;
}
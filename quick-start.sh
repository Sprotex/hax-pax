ensure_packages() {
  packages=(
    "build-essential"
    "cmake"
    "exuberant-ctags"
    "g++-arm-linux-gnueabi"
    "gcc-arm-linux-gnueabi"
    "gcc-arm-none-eabi"
    "gcc"
    "libsdl2-2.0-0"
    "libsdl2-dev"
    "libssl-dev"
    "pkg-config"
    "python3-dev"
    "python3-pip"
    "python3-venv"
    "swig"
    "telnet"
    "unzip"
    "usbutils"
  )

  packages_to_install=()

  for package in "${packages[@]}"; do
    if ! dpkg -l | grep -q "$package" || $force_install; then
      packages_to_install+=("$package")
    fi
  done

  if [ ${#packages_to_install[@]} -eq 0 ]; then
    echo "All necessary packages are already installed."
  else
    echo "The following packages will be installed: ${packages_to_install[@]}"
    sudo apt update && sudo apt install "${packages_to_install[@]}"
  fi
}

ensure_uploader() {
  folder_name="prolin-xcb-client"
  zip_file="master.zip"
  url="https://git.lsd.cat/g/prolin-xcb-client/archive/$zip_file"
  if [ ! -d "../$folder_name" ]; then
    echo "The uploader is not installed. Downloading..."
    wget "$url" && sudo unzip "$zip_file" -d ../
    rm "$zip_file"
  else
    echo "The uploader is already installed."
  fi
}

install_python_dependencies() {
  if [ ! -d "env" ]; then
    python3 -m venv env
  fi
  source env/bin/activate
  pip3 install -r requirements.txt
}

display_help_message() {
  echo "Usage: quick-start.sh [-f | --force] [-h | --help]"
  echo "  -f or --force to force install all packages"
  echo "  -h or --help to show help message"
}

parse_arguments() {
  for arg in "$@"; do
    case $arg in
    -f | --force)
      force_install=true
      shift
      ;;
    -h | --help)
      display_help_message
      exit 0
      ;;
    *)
      echo "Invalid argument: $arg"
      display_help_message
      exit 1
      ;;
    esac
  done
}

force_install=false

parse_arguments "$@"
ensure_packages
ensure_uploader
install_python_dependencies
